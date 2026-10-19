// Intercom moto - étape 3 : musique Bluetooth + intercom ESP-NOW dans le même boîtier
//
// Fonctionnement :
//  1. Le téléphone envoie la musique en Bluetooth (A2DP), comme à l'étape 1.
//  2. En même temps, la voix circule entre les deux intercoms en ESP-NOW, comme à l'étape 2
//     (déclenchement à la voix uniquement).
//  3. Une tâche unique mélange les deux sons et alimente l'ampli. Quand l'autre parle,
//     la musique baisse automatiquement puis remonte.
//
// La voix arrive à 16 kHz et la musique à 44,1 kHz (parfois 48 kHz) : la voix est
// rééchantillonnée à la volée avant le mélange.
//
// Le Bluetooth et le Wi-Fi partagent la même antenne et se relaient. Le tampon
// anti-saccades de la voix est donc plus grand qu'à l'étape 2 (90 ms au lieu de 45).
//
// Boutons (entre la broche et GND) :
//   A : court = lecture / pause, long = couper / rétablir le micro
//   B : court = volume +, long = piste suivante
//   C : court = volume -, long = piste précédente

#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>
#include "BluetoothA2DPSink.h"
#include "driver/i2s.h"
#include "anneau.h"
#include "bouton.h"
#include "ulaw.h"
#include "config.h"

// ---------- Broches (identiques aux étapes 1 et 2) ----------
#define BROCHE_I2S_BCLK  26
#define BROCHE_I2S_LRC   25
#define BROCHE_I2S_DOUT  22
#define BROCHE_MIC_SCK   14
#define BROCHE_MIC_WS    15
#define BROCHE_MIC_SD    34
#define BROCHE_LED        2
// Le haut-parleur doit rester sur le port 0 : la bibliothèque A2DP y règle la fréquence.
#define PORT_HP          I2S_NUM_0
#define PORT_MIC         I2S_NUM_1

// ---------- Audio ----------
#define FREQ_VOIX        16000
#define ECH_PAR_PAQUET   240                     // 15 ms de voix par paquet
#define PREREMP_VOIX     (6 * ECH_PAR_PAQUET)    // 90 ms en réserve avant de jouer
#define RETARD_MAXI_VOIX (20 * ECH_PAR_PAQUET)   // au-delà de 300 ms de retard, on rattrape
#define PREREMP_MUSIQUE  2048                    // ~46 ms de musique en réserve
#define BLOC_SORTIE      256                     // échantillons envoyés à l'ampli à chaque tour
#define MAINTIEN_VOX_MS  800
#define DUREE_VOIX_MS    400                     // la musique remonte 0,4 s après le dernier paquet
#define MAGIE            0xB1

struct __attribute__((packed)) Paquet {
  uint8_t magie;
  uint8_t groupe;
  uint16_t numero;
  uint8_t son[ECH_PAR_PAQUET];
};

Bouton boutonA(32), boutonB(33), boutonC(27);
BluetoothA2DPSink a2dp;
Anneau<4096> voixRecue;      // 16 kHz mono
Anneau<8192> musique;        // fréquence du téléphone, mono
const uint8_t DIFFUSION[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

volatile int volume = VOLUME_DEPART;
volatile int32_t gainQ15 = 0;
volatile uint32_t frequenceSortie = 44100;
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
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;   // même format que la bibliothèque A2DP
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.dma_buf_count = 6;
  cfg.dma_buf_len = BLOC_SORTIE;
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
  cfg.sample_rate = FREQ_VOIX;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;
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

// ---------- Bluetooth : musique ----------
// Appelée par la pile Bluetooth : stéréo 16 bits -> mono, rangé dans le tampon musique.
void recevoirMusique(const uint8_t* donnees, uint32_t taille) {
  static int16_t mono[256];
  const int16_t* ech = (const int16_t*)donnees;
  uint32_t trames = taille / 4;
  while (trames > 0) {
    uint32_t n = min<uint32_t>(trames, 256);
    for (uint32_t i = 0; i < n; i++) mono[i] = (int16_t)(((int32_t)ech[2 * i] + ech[2 * i + 1]) / 2);
    musique.ecrire(mono, n);
    ech += 2 * n;
    trames -= n;
  }
}

void changerFrequence(uint16_t frequence) {
  Serial.printf("Fréquence musique : %u Hz\n", frequence);
  frequenceSortie = frequence;
  musique.vider();
  i2s_set_sample_rates(PORT_HP, frequence);
}

void connexionChangee(esp_a2d_connection_state_t etat, void*) {
  if (etat == ESP_A2D_CONNECTION_STATE_CONNECTED) Serial.println("Téléphone connecté");
  if (etat == ESP_A2D_CONNECTION_STATE_DISCONNECTED) Serial.println("Téléphone déconnecté");
}

// ---------- ESP-NOW : voix ----------
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
  // Avec le Bluetooth actif, le Wi-Fi doit garder son économie d'énergie (WIFI_PS_NONE est refusé).
  esp_wifi_set_channel(CANAL_WIFI, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("Erreur : ESP-NOW ne démarre pas");
    return;
  }
  esp_now_register_recv_cb(paquetRecu);

  esp_now_peer_info_t pair = {};
  memcpy(pair.peer_addr, DIFFUSION, 6);
  pair.channel = CANAL_WIFI;
  pair.ifidx = WIFI_IF_STA;
  pair.encrypt = false;
  esp_now_add_peer(&pair);
  Serial.printf("ESP-NOW prêt, canal %d, groupe %d\n", CANAL_WIFI, GROUPE_INTERCOM);
}

// ---------- Tâche micro : identique à l'étape 2, en VOX ----------
void tacheMicro(void*) {
  static int32_t brut[ECH_PAR_PAQUET];
  Paquet paquet;
  paquet.magie = MAGIE;
  paquet.groupe = GROUPE_INTERCOM;
  uint16_t numero = 0;
  int32_t precedent = 0, filtre = 0;
  uint32_t finVoxMs = 0;

  for (;;) {
    size_t lus = 0;
    i2s_read(PORT_MIC, brut, sizeof(brut), &lus, portMAX_DELAY);
    if (lus / 4 < ECH_PAR_PAQUET) continue;

    int32_t somme = 0;
    for (int i = 0; i < ECH_PAR_PAQUET; i++) {
      int32_t x = brut[i] >> 14;
      filtre = x - precedent + ((filtre * 252) >> 8);
      precedent = x;
      int32_t v = constrain((filtre >> 2) * GAIN_MICRO, -32767, 32767);
      somme += abs(v);
      paquet.son[i] = ulawEncoder((int16_t)v);
    }
    niveauMicro = somme / ECH_PAR_PAQUET;

    uint32_t maintenant = millis();
    if (niveauMicro > SEUIL_VOX) finVoxMs = maintenant + MAINTIEN_VOX_MS;
    bool parle = !microCoupe && (int32_t)(finVoxMs - maintenant) > 0;
    enEmission = parle;
    if (parle) {
      paquet.numero = numero++;
      if (esp_now_send(DIFFUSION, (uint8_t*)&paquet, sizeof(paquet)) == ESP_OK) paquetsEnvoyes++;
    }
  }
}

// ---------- Rééchantillonneur 16 kHz -> fréquence de sortie (interpolation linéaire) ----------
class Reechantillonneur {
public:
  // Produit un échantillon de voix à la fréquence de sortie.
  int16_t suivant(uint32_t freqSortie) {
    // pas = 16000 / freqSortie, en virgule fixe 16.16
    uint32_t pas = ((uint32_t)FREQ_VOIX << 16) / freqSortie;
    phase += pas;
    while (phase >= 0x10000) {
      phase -= 0x10000;
      avant = apres;
      apres = lireEntree();
    }
    // phase réduite à 15 bits pour que le produit tienne dans 32 bits
    return (int16_t)(avant + (((int32_t)(apres - avant) * (int32_t)(phase >> 1)) >> 15));
  }

private:
  // Tampon d'entrée local pour ne pas verrouiller l'anneau à chaque échantillon
  int16_t lireEntree() {
    if (pos >= nb) {
      pos = 0;
      nb = 0;
      size_t dispo = voixRecue.disponible();
      if (dispo > RETARD_MAXI_VOIX) {
        int16_t poubelle[ECH_PAR_PAQUET];
        while (voixRecue.disponible() > PREREMP_VOIX) voixRecue.lire(poubelle, ECH_PAR_PAQUET);
      }
      if (!enLecture && dispo >= PREREMP_VOIX) enLecture = true;
      if (enLecture) {
        nb = voixRecue.lire(entree, 64);
        if (nb == 0) enLecture = false;
      }
      if (nb == 0) return 0;
    }
    return entree[pos++];
  }

  int16_t entree[64];
  size_t pos = 0, nb = 0;
  bool enLecture = false;
  uint32_t phase = 0;
  int16_t avant = 0, apres = 0;
};

// ---------- Tâche de sortie : mélange musique + voix ----------
void tacheSortie(void*) {
  static int16_t bloc[BLOC_SORTIE];
  static int16_t stereo[BLOC_SORTIE * 2];
  Reechantillonneur voix;
  bool musiqueEnLecture = false;
  int32_t gainMusique = 32768;   // descend pendant la voix, remonte ensuite

  for (;;) {
    // Musique : on attend d'avoir une petite réserve pour éviter les craquements
    size_t n = 0;
    if (!musiqueEnLecture && musique.disponible() >= PREREMP_MUSIQUE) musiqueEnLecture = true;
    if (musiqueEnLecture) {
      n = musique.lire(bloc, BLOC_SORTIE);
      if (n < BLOC_SORTIE) musiqueEnLecture = false;
    }
    for (size_t i = n; i < BLOC_SORTIE; i++) bloc[i] = 0;

    // Baisse de la musique quand l'autre parle, en douceur
    bool voixActive = millis() - dernierPaquetMs < DUREE_VOIX_MS;
    int32_t cible = voixActive ? (int32_t)MUSIQUE_PENDANT_VOIX * 32768 / 100 : 32768;

    uint32_t freq = frequenceSortie;
    int32_t g = gainQ15;
    for (int i = 0; i < BLOC_SORTIE; i++) {
      gainMusique += (cible - gainMusique) >> 9;
      int32_t m = ((int32_t)bloc[i] * gainMusique) >> 15;
      int32_t v = voix.suivant(freq);
      int32_t s = ((m + v) * g) >> 15;
      s = constrain(s, -32767, 32767);
      stereo[2 * i] = (int16_t)s;
      stereo[2 * i + 1] = (int16_t)s;
    }
    size_t ecrits;
    i2s_write(PORT_HP, stereo, sizeof(stereo), &ecrits, portMAX_DELAY);
  }
}

// ---------- Programme principal ----------
void setup() {
  Serial.begin(115200);
  Serial.println("\nIntercom moto - étape 3 : musique + intercom");
  pinMode(BROCHE_LED, OUTPUT);
  boutonA.begin();
  boutonB.begin();
  boutonC.begin();
  appliquerVolume(VOLUME_DEPART);

  initHautParleur();
  initMicro();
  initRadio();

  a2dp.set_stream_reader(recevoirMusique, false);  // false : c'est nous qui gérons l'I2S
  a2dp.set_sample_rate_callback(changerFrequence);
  a2dp.set_on_connection_state_changed(connexionChangee);
  a2dp.start(NOM_BLUETOOTH);

  xTaskCreatePinnedToCore(tacheMicro, "micro", 6144, nullptr, 5, nullptr, 1);
  xTaskCreatePinnedToCore(tacheSortie, "sortie", 6144, nullptr, 6, nullptr, 1);
  Serial.printf("Prêt : « %s » en Bluetooth, intercom groupe %d\n", NOM_BLUETOOTH, GROUPE_INTERCOM);
  Serial.printf("Mémoire libre : %u octets\n", ESP.getFreeHeap());
}

void loop() {
  switch (boutonA.lire()) {
    case Appui::COURT:
      if (a2dp.get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED) a2dp.pause();
      else a2dp.play();
      break;
    case Appui::LONG:
      microCoupe = !microCoupe;
      Serial.println(microCoupe ? "Micro coupé" : "Micro rétabli");
      break;
    default: break;
  }
  switch (boutonB.lire()) {
    case Appui::COURT: appliquerVolume(volume + 10); break;
    case Appui::LONG:  a2dp.next(); break;
    default: break;
  }
  switch (boutonC.lire()) {
    case Appui::COURT: appliquerVolume(volume - 10); break;
    case Appui::LONG:  a2dp.previous(); break;
    default: break;
  }

  // LED : fixe si le téléphone est connecté, clignote rapidement quand l'autre parle
  bool voix = millis() - dernierPaquetMs < 200;
  bool tel = a2dp.is_connected();
  digitalWrite(BROCHE_LED, voix ? (millis() / 100) % 2 : tel);

  static uint32_t dernierAffichage = 0;
  if (millis() - dernierAffichage >= 1000) {
    dernierAffichage = millis();
    Serial.printf("Micro %4ld %s | envoyés %lu | reçus %lu | perdus %lu | musique en réserve %u\n",
                  (long)niveauMicro, enEmission ? "(émission)" : "           ",
                  (unsigned long)paquetsEnvoyes, (unsigned long)paquetsRecus,
                  (unsigned long)paquetsPerdus, (unsigned)musique.disponible());
  }
  delay(5);
}
