#include "voix.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include <freertos/task.h>
#include "driver/adc.h"
#include "driver/i2s.h"
#include "esp_hf_client_api.h"
#include "audio.h"
#include "parametres.h"

#define PORT_MICRO I2S_NUM_0      // seul l'I2S0 sait lire l'ADC
#define FREQ_MICRO 16000          // l'ADC tourne à 16 kHz, ramené à 8 kHz (moyenne) si besoin
#define BLOC_MICRO 160            // 10 ms à 16 kHz

#define TAILLE_HP 8192            // 512 ms à 8 kHz
#define TAILLE_TEL 4096           // 256 ms à 8 kHz

static RingbufHandle_t s_versHp = nullptr;     // téléphone -> ampli
static RingbufHandle_t s_versTel = nullptr;    // micro -> téléphone
static volatile bool s_active = false;
static volatile uint32_t s_frequence = 8000;
static volatile int32_t s_gainQ15 = 32768;
static volatile uint8_t s_gainMicro = GAIN_MICRO;
static TaskHandle_t s_tacheHp = nullptr, s_tacheMicro = nullptr;

bool voixActive() { return s_active; }
void voixSetGainMicro(uint8_t g) { s_gainMicro = g < 1 ? 1 : (g > 32 ? 32 : g); }
uint8_t voixGainMicro() { return s_gainMicro; }
uint32_t voixFrequence() { return s_frequence; }

void voixSetVolume(uint8_t v) {
  if (v > 15) v = 15;
  s_gainQ15 = (int32_t)v * v * 32768 / 225;   // courbe quadratique, 15 = 1,0
}

static void vider(RingbufHandle_t rb) {
  size_t n;
  void *p;
  while ((p = xRingbufferReceiveUpTo(rb, &n, 0, 4096)) != nullptr) vRingbufferReturnItem(rb, p);
}

// ---------- téléphone -> haut-parleur ----------

void voixRecu(const uint8_t *donnees, uint32_t taille) {
  if (!s_active) return;
  xRingbufferSend(s_versHp, donnees, taille, 0);   // tampon plein : on perd ce paquet
  esp_hf_client_outgoing_data_ready();              // la pile vient chercher le son du micro
}

static size_t niveau(RingbufHandle_t rb, size_t taille) {
  return taille - xRingbufferGetCurFreeSize(rb);
}

// Lit exactement n octets (moins si le tampon n'en a pas assez) ; renvoie le nombre lu.
static size_t lire(RingbufHandle_t rb, uint8_t *dst, size_t n) {
  size_t fait = 0;
  while (fait < n) {
    size_t k = 0;
    uint8_t *p = (uint8_t *)xRingbufferReceiveUpTo(rb, &k, 0, n - fait);
    if (!p) break;
    memcpy(dst + fait, p, k);
    vRingbufferReturnItem(rb, p);
    fait += k;
  }
  return fait;
}

// Écrit la voix par tampons DMA entiers : l'écriture bloque jusqu'à ce qu'un tampon se
// libère, ce qui cale le rythme sur l'ampli. S'il manque du son, on complète par du silence.
static void tacheHautParleur(void *) {
  static int16_t bloc[256];
  for (;;) {
    if (!s_active) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      continue;
    }
    // Réserve de 90 ms avant de jouer : 48 ms pour remplir les tampons de l'ampli,
    // le reste pour absorber l'arrivée irrégulière des paquets.
    size_t reserve = s_frequence * 2 * 90 / 1000;
    while (s_active && niveau(s_versHp, TAILLE_HP) < reserve) vTaskDelay(pdMS_TO_TICKS(5));
    uint32_t manques = 0;
    while (s_active) {
      if (audioMode() != ModeAudio::APPEL) {   // sonnerie encore en cours : on jette la voix
        size_t n = 0;
        void *p = xRingbufferReceiveUpTo(s_versHp, &n, pdMS_TO_TICKS(20), 512);
        if (p) vRingbufferReturnItem(s_versHp, p);
        continue;
      }
      size_t trames = audioTailleBloc();
      if (trames > 256) trames = 256;
      // Trop de retard (le téléphone envoie un peu plus vite que l'ampli ne joue) : on rattrape
      if (niveau(s_versHp, TAILLE_HP) > s_frequence * 2 / 5) {   // > 200 ms
        lire(s_versHp, (uint8_t *)bloc, trames * 2);
      }
      size_t lu = lire(s_versHp, (uint8_t *)bloc, trames * 2);
      if (lu < trames * 2) {
        memset((uint8_t *)bloc + lu, 0, trames * 2 - lu);
        if (++manques % 50 == 1) Serial.printf("[Voix] Son du téléphone en retard (%u fois)\n", (unsigned)manques);
      }
      audioEcrireMono(bloc, trames, s_gainQ15, ModeAudio::APPEL);
    }
  }
}

// ---------- micro -> téléphone ----------

// Appelée par la pile Bluetooth quand elle envoie un paquet. Le micro produit par blocs de
// 10 ms : on attend d'avoir 30 ms d'avance avant d'en envoyer, sinon on enverrait un
// mélange de son et de trous (voix hachée).
static volatile bool s_amorce = true;

uint32_t voixAEnvoyer(uint8_t *donnees, uint32_t taille) {
  if (s_amorce) {
    if (niveau(s_versTel, TAILLE_TEL) < s_frequence * 2 * 30 / 1000) {
      memset(donnees, 0, taille);
      return taille;
    }
    s_amorce = false;
  }
  size_t fait = lire(s_versTel, donnees, taille);
  if (fait < taille) {
    memset(donnees + fait, 0, taille - fait);
    s_amorce = true;   // plus de son d'avance : on se refait une réserve
  }
  return taille;
}

#if AVEC_MICRO
// Micro analogique (module MAX9814, MAX4466…) sur une entrée de l'ADC1, lu par le
// « mode ADC » de l'I2S0 : l'ESP32 échantillonne seul, en DMA, sans charger le processeur.
// (L'ampli est sur l'I2S1.)
static adc1_channel_t canalAdc(int gpio) {
  switch (gpio) {
    case 36: return ADC1_CHANNEL_0;
    case 37: return ADC1_CHANNEL_1;
    case 38: return ADC1_CHANNEL_2;
    case 39: return ADC1_CHANNEL_3;
    case 32: return ADC1_CHANNEL_4;
    case 33: return ADC1_CHANNEL_5;
    case 34: return ADC1_CHANNEL_6;
    case 35: return ADC1_CHANNEL_7;
    default: return ADC1_CHANNEL_MAX;   // pas une broche de l'ADC1
  }
}

static bool microOuvrir() {
  adc1_channel_t canal = canalAdc(BROCHE_MICRO);
  if (canal == ADC1_CHANNEL_MAX) {
    Serial.printf("[Voix] GPIO%d n'est pas une entrée de l'ADC1 (32 à 39) : pas de micro\n", BROCHE_MICRO);
    return false;
  }
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX | I2S_MODE_ADC_BUILT_IN);
  cfg.sample_rate = FREQ_MICRO;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_ONLY_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_MSB;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = BLOC_MICRO;
  cfg.use_apll = false;
  if (i2s_driver_install(PORT_MICRO, &cfg, 0, nullptr) != ESP_OK) {
    Serial.println("[Voix] Impossible de démarrer le micro (I2S0 en mode ADC)");
    return false;
  }
  i2s_set_adc_mode(ADC_UNIT_1, canal);
  adc1_config_channel_atten(canal, ADC_ATTEN_DB_11);   // entrée de 0 à environ 3 V
  i2s_adc_enable(PORT_MICRO);
  return true;
}

static void microFermer() {
  i2s_adc_disable(PORT_MICRO);
  i2s_driver_uninstall(PORT_MICRO);
}

static void tacheMicro(void *) {
  static uint16_t brut[BLOC_MICRO];
  static int16_t sortie[BLOC_MICRO];
  bool ouvert = false;
  bool premier = true;
  int32_t dcEntree = 0, dcSortie = 0;   // filtre passe-haut : retire la tension de repos du module
  // Fréquence réelle de l'ADC, mesurée la première seconde (le mode ADC de l'I2S ne
  // tombe pas toujours pile sur la fréquence demandée).
  uint32_t mesureDebut = 0, mesureNb = 0, freqReelle = FREQ_MICRO;
  int32_t somme = 0;
  uint32_t nbSomme = 0;

  for (;;) {
    if (!s_active) {
      if (ouvert) {
        microFermer();
        ouvert = false;
      }
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      continue;
    }
    if (!ouvert) {
      if (!microOuvrir()) { vTaskDelay(pdMS_TO_TICKS(1000)); continue; }
      ouvert = true;
      premier = true;
      mesureDebut = millis();
      mesureNb = 0;
      somme = 0;
      nbSomme = 0;
    }
    size_t lus = 0;
    i2s_read(PORT_MICRO, brut, sizeof brut, &lus, pdMS_TO_TICKS(50));
    size_t nb = lus / 2;
    if (nb == 0) continue;

    if (mesureDebut) {
      mesureNb += nb;
      uint32_t ecoule = millis() - mesureDebut;
      if (ecoule >= 1000) {
        freqReelle = (uint32_t)((uint64_t)mesureNb * 1000 / ecoule);
        mesureDebut = 0;
        Serial.printf("[Voix] Micro : %u échantillons/s\n", (unsigned)freqReelle);
      }
    }
    // Nombre d'échantillons du micro moyennés pour un échantillon envoyé au téléphone
    uint32_t facteur = (freqReelle + s_frequence / 2) / s_frequence;
    if (facteur < 1) facteur = 1;

    size_t n = 0;
    for (size_t i = 0; i < nb; i++) {
      int32_t v = brut[i] & 0x0FFF;   // 12 bits ; les 4 bits du haut donnent le canal
      if (premier) { dcEntree = v << 4; premier = false; }
      somme += v;
      if (++nbSomme < facteur) continue;
      int32_t x = (somme << 4) / (int32_t)nbSomme;   // 12 -> 16 bits
      somme = 0;
      nbSomme = 0;
      int32_t y = x - dcEntree + dcSortie - (dcSortie >> 8);
      dcEntree = x;
      dcSortie = y;
      int32_t z = y * s_gainMicro / 4;
      if (z > 32767) z = 32767;
      if (z < -32768) z = -32768;
      sortie[n++] = (int16_t)z;
    }
    // Trop d'avance (horloges du micro et du téléphone un peu différentes) : on jette 10 ms
    if (niveau(s_versTel, TAILLE_TEL) > s_frequence * 2 / 8) lire(s_versTel, (uint8_t *)brut, s_frequence / 50);
    if (n) xRingbufferSend(s_versTel, sortie, n * 2, 0);
  }
}
#endif

// ---------- démarrage / arrêt ----------

void voixInit() {
  s_versHp = xRingbufferCreate(TAILLE_HP, RINGBUF_TYPE_BYTEBUF);
  s_versTel = xRingbufferCreate(TAILLE_TEL, RINGBUF_TYPE_BYTEBUF);
  voixSetVolume(VOLUME_APPEL_DEPART);
  xTaskCreate(tacheHautParleur, "voix_hp", VOIX_PILE, nullptr, VOIX_PRIO, &s_tacheHp);
#if AVEC_MICRO
  xTaskCreate(tacheMicro, "voix_micro", VOIX_PILE, nullptr, VOIX_PRIO, &s_tacheMicro);
#endif
}

void voixDemarrer(uint32_t frequence) {
  vider(s_versHp);
  vider(s_versTel);
  s_amorce = true;
  s_frequence = frequence;
  s_active = true;
  xTaskNotifyGive(s_tacheHp);
  if (s_tacheMicro) xTaskNotifyGive(s_tacheMicro);
  Serial.printf("[Voix] Liaison audio d'appel ouverte (%u Hz)%s\n", (unsigned)frequence,
                AVEC_MICRO ? "" : ", pas de micro");
}

void voixArreter() {
  if (!s_active) return;
  s_active = false;
  Serial.println("[Voix] Liaison audio d'appel fermée");
}
