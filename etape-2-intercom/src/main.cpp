// Intercom moto - étape 2 : intercom entre deux casques par ESP-NOW
//
// Fonctionnement :
//  1. Le micro numérique INMP441 (I2S) capte la voix à 16 kHz.
//  2. Quand tu parles (VOX) ou que tu appuies sur A, le son est compressé en µ-law
//     et envoyé en ESP-NOW par paquets de 15 ms, sans box ni téléphone.
//  3. L'autre intercom reçoit les paquets, les met dans un petit tampon anti-saccades
//     et les joue sur ses haut-parleurs (MAX98357A).
//  Les deux peuvent parler en même temps (full duplex).
//
// Boutons (entre la broche et GND) :
//   A : appuyer pour parler (si MODE_APPUYER_POUR_PARLER = 1)
//       sinon court = couper / rétablir le micro
//   B : court = volume +
//   C : court = volume -

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "driver/i2s.h"
#include "anneau.h"
#include "bouton.h"
#include "ulaw.h"
#include "config.h"

// ---------- Broches ----------
#define BROCHE_I2S_BCLK  26   // MAX98357A : BCLK
#define BROCHE_I2S_LRC   25   // MAX98357A : LRC
#define BROCHE_I2S_DOUT  22   // MAX98357A : DIN
#define BROCHE_MIC_SCK   14   // INMP441 : SCK
#define BROCHE_MIC_WS    15   // INMP441 : WS
#define BROCHE_MIC_SD    34   // INMP441 : SD (GPIO34 = entrée seule, parfait ici)
#define BROCHE_LED        2   // allumée quand on reçoit la voix de l'autre
#define PORT_HP          I2S_NUM_0
#define PORT_MIC         I2S_NUM_1

// ---------- Audio ----------
#define FREQUENCE        16000
#define ECH_PAR_PAQUET   240                     // 15 ms de son par paquet
#define PREREMPLISSAGE   (3 * ECH_PAR_PAQUET)    // 45 ms en réserve avant de jouer
#define RETARD_MAXI      (16 * ECH_PAR_PAQUET)   // au-delà de 240 ms de retard, on rattrape
#define MAINTIEN_VOX_MS  800                     // le micro reste ouvert 0,8 s après la dernière syllabe
#define MAGIE            0xB1

struct __attribute__((packed)) Paquet {
  uint8_t magie;
  uint8_t groupe;
  uint16_t numero;
  uint8_t son[ECH_PAR_PAQUET];
};

Bouton boutonA(32), boutonB(33), boutonC(27);
Anneau<4096> voixRecue;
const uint8_t DIFFUSION[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

volatile int volume = VOLUME_DEPART;
volatile int32_t gainQ15 = 0;
volatile bool microCoupe = false;
volatile uint32_t dernierPaquetMs = 0;
volatile uint32_t paquetsRecus = 0, paquetsPerdus = 0, paquetsEnvoyes = 0;
volatile int32_t niveauMicro = 0;
volatile bool enEmission = false;

void appliquerVolume(int nouveau) {
  volume = constrain(nouveau, 0, 100);
  gainQ15 = (int32_t)volume * volume * 32768 / 10000;
  Serial.printf("Volume : %d %%\n", volume);
}

// ---------- Initialisation I2S ----------
void initHautParleur() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = FREQUENCE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 6;
  cfg.dma_buf_len = ECH_PAR_PAQUET;
  cfg.tx_desc_auto_clear = true;
  i2s_driver_install(PORT_HP, &cfg, 0, nullptr);

  i2s_pin_config_t broches = {};
  broches.mck_io_num = I2S_PIN_NO_CHANGE;
  broches.bck_io_num = BROCHE_I2S_BCLK;
  broches.ws_io_num = BROCHE_I2S_LRC;
  broches.data_out_num = BROCHE_I2S_DOUT;
  broches.data_in_num = I2S_PIN_NO_CHANGE;
  i2s_set_pin(PORT_HP, &broches);
}

void initMicro() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = FREQUENCE;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;   // l'INMP441 envoie 24 bits dans 32
  cfg.channel_format = MICRO_CANAL_DROIT ? I2S_CHANNEL_FMT_ONLY_RIGHT : I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 6;
  cfg.dma_buf_len = ECH_PAR_PAQUET;
  i2s_driver_install(PORT_MIC, &cfg, 0, nullptr);

  i2s_pin_config_t broches = {};
  broches.mck_io_num = I2S_PIN_NO_CHANGE;
  broches.bck_io_num = BROCHE_MIC_SCK;
  broches.ws_io_num = BROCHE_MIC_WS;
  broches.data_out_num = I2S_PIN_NO_CHANGE;
  broches.data_in_num = BROCHE_MIC_SD;
  i2s_set_pin(PORT_MIC, &broches);
}

// ---------- Radio ESP-NOW ----------
// Appelée par la tâche Wi-Fi à chaque paquet reçu : on décode et on range dans le tampon.
void paquetRecu(const uint8_t* mac, const uint8_t* donnees, int taille) {
  static uint16_t numeroAttendu = 0;
  if (taille != sizeof(Paquet)) return;
  const Paquet* p = (const Paquet*)donnees;
  if (p->magie != MAGIE || p->groupe != GROUPE_INTERCOM) return;

  uint32_t maintenant = millis();
  if (maintenant - dernierPaquetMs < 1000 && p->numero != numeroAttendu) {
    paquetsPerdus += (uint16_t)(p->numero - numeroAttendu);
  }
  numeroAttendu = p->numero + 1;
  dernierPaquetMs = maintenant;
  paquetsRecus++;

  int16_t pcm[ECH_PAR_PAQUET];
  for (int i = 0; i < ECH_PAR_PAQUET; i++) pcm[i] = ulawDecoder(p->son[i]);
  voixRecue.ecrire(pcm, ECH_PAR_PAQUET);
}

void initRadio() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_ps(WIFI_PS_NONE);  // pas d'économie d'énergie : moins de latence
#if LONGUE_PORTEE
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_LR);
#endif
  esp_wifi_set_channel(CANAL_WIFI, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur : ESP-NOW ne démarre pas");
    return;
  }
  esp_now_register_recv_cb(paquetRecu);

  // Envoi en diffusion : pas besoin de connaître l'adresse MAC de l'autre intercom
  esp_now_peer_info_t pair = {};
  memcpy(pair.peer_addr, DIFFUSION, 6);
  pair.channel = CANAL_WIFI;
  pair.ifidx = WIFI_IF_STA;
  pair.encrypt = false;
  esp_now_add_peer(&pair);
  Serial.printf("ESP-NOW prêt, canal %d, groupe %d, adresse %s\n",
                CANAL_WIFI, GROUPE_INTERCOM, WiFi.macAddress().c_str());
}

// ---------- Tâche micro : capture, VOX, envoi ----------
void tacheMicro(void*) {
  static int32_t brut[ECH_PAR_PAQUET];
  Paquet paquet;
  paquet.magie = MAGIE;
  paquet.groupe = GROUPE_INTERCOM;
  uint16_t numero = 0;
  int32_t precedent = 0, filtre = 0;   // filtre passe-haut : retire le continu et le grondement
  uint32_t finVoxMs = 0;

  for (;;) {
    size_t lus = 0;
    i2s_read(PORT_MIC, brut, sizeof(brut), &lus, portMAX_DELAY);
    int n = lus / 4;
    if (n < ECH_PAR_PAQUET) continue;

    int32_t somme = 0;
    for (int i = 0; i < ECH_PAR_PAQUET; i++) {
      int32_t x = brut[i] >> 14;                       // 18 bits utiles
      filtre = x - precedent + ((filtre * 252) >> 8);  // coupe sous ~100 Hz
      precedent = x;
      int32_t v = (filtre >> 2) * GAIN_MICRO;
      v = constrain(v, -32767, 32767);
      somme += abs(v);
      paquet.son[i] = ulawEncoder((int16_t)v);
    }
    niveauMicro = somme / ECH_PAR_PAQUET;

    uint32_t maintenant = millis();
    bool parle;
#if MODE_APPUYER_POUR_PARLER
    parle = boutonA.estEnfonce();
#else
    if (niveauMicro > SEUIL_VOX) finVoxMs = maintenant + MAINTIEN_VOX_MS;
    parle = !microCoupe && (int32_t)(finVoxMs - maintenant) > 0;
#endif
    enEmission = parle;
    if (parle) {
      paquet.numero = numero++;
      if (esp_now_send(DIFFUSION, (uint8_t*)&paquet, sizeof(paquet)) == ESP_OK) paquetsEnvoyes++;
    }
  }
}

// ---------- Tâche haut-parleur : tampon anti-saccades et lecture ----------
void tacheHautParleur(void*) {
  static int16_t mono[ECH_PAR_PAQUET];
  static int16_t stereo[ECH_PAR_PAQUET * 2];
  bool enLecture = false;

  for (;;) {
    size_t dispo = voixRecue.disponible();
    if (dispo > RETARD_MAXI) {
      // Trop de retard accumulé : on jette le surplus pour rester en direct
      int16_t poubelle[ECH_PAR_PAQUET];
      while (voixRecue.disponible() > PREREMPLISSAGE) voixRecue.lire(poubelle, ECH_PAR_PAQUET);
    }
    if (!enLecture && dispo >= PREREMPLISSAGE) enLecture = true;

    size_t n = 0;
    if (enLecture) {
      n = voixRecue.lire(mono, ECH_PAR_PAQUET);
      if (n < ECH_PAR_PAQUET) enLecture = false;  // tampon vide : on attend de le remplir à nouveau
    }
    for (size_t i = n; i < ECH_PAR_PAQUET; i++) mono[i] = 0;

    int32_t g = gainQ15;
    for (int i = 0; i < ECH_PAR_PAQUET; i++) {
      int16_t v = (int16_t)(((int32_t)mono[i] * g) >> 15);
      stereo[2 * i] = v;
      stereo[2 * i + 1] = v;
    }
    size_t ecrits;
    i2s_write(PORT_HP, stereo, sizeof(stereo), &ecrits, portMAX_DELAY);  // bloque au rythme de l'I2S
  }
}

// ---------- Programme principal ----------
void setup() {
  Serial.begin(115200);
  Serial.println("\nIntercom moto - étape 2 : intercom ESP-NOW");
  pinMode(BROCHE_LED, OUTPUT);
  boutonA.begin();
  boutonB.begin();
  boutonC.begin();
  appliquerVolume(VOLUME_DEPART);

  initHautParleur();
  initMicro();
  initRadio();

  xTaskCreatePinnedToCore(tacheMicro, "micro", 6144, nullptr, 5, nullptr, 1);
  xTaskCreatePinnedToCore(tacheHautParleur, "hp", 6144, nullptr, 6, nullptr, 1);
#if MODE_APPUYER_POUR_PARLER
  Serial.println("Mode : appuyer sur A pour parler");
#else
  Serial.printf("Mode : déclenchement à la voix, seuil %d\n", SEUIL_VOX);
#endif
}

void loop() {
#if !MODE_APPUYER_POUR_PARLER
  if (boutonA.lire() == Appui::COURT) {
    microCoupe = !microCoupe;
    Serial.println(microCoupe ? "Micro coupé" : "Micro rétabli");
  }
#else
  boutonA.lire();  // met à jour estEnfonce()
#endif
  if (boutonB.lire() == Appui::COURT) appliquerVolume(volume + 10);
  if (boutonC.lire() == Appui::COURT) appliquerVolume(volume - 10);

  digitalWrite(BROCHE_LED, millis() - dernierPaquetMs < 200);

  // Une ligne par seconde sur le moniteur série pour régler le VOX et vérifier la liaison
  static uint32_t dernierAffichage = 0;
  if (millis() - dernierAffichage >= 1000) {
    dernierAffichage = millis();
    Serial.printf("Micro : niveau %4ld %s | envoyés %lu | reçus %lu | perdus %lu\n",
                  (long)niveauMicro, enEmission ? "(émission)" : "           ",
                  (unsigned long)paquetsEnvoyes, (unsigned long)paquetsRecus,
                  (unsigned long)paquetsPerdus);
  }
  delay(5);
}
