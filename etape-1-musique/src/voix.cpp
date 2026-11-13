#include "voix.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/ringbuf.h>
#include <freertos/task.h>
#include "driver/i2s.h"
#include "esp_hf_client_api.h"
#include "audio.h"
#include "parametres.h"

#define PORT_MICRO I2S_NUM_1
#define FREQ_MICRO 16000          // l'INMP441 tourne à 16 kHz, ramené à 8 kHz si besoin
#define BLOC_MICRO 160            // 10 ms à 16 kHz

static RingbufHandle_t s_versHp = nullptr;     // téléphone -> ampli
static RingbufHandle_t s_versTel = nullptr;    // micro -> téléphone
static volatile bool s_active = false;
static volatile uint32_t s_frequence = 8000;
static volatile int32_t s_gainQ15 = 32768;
static TaskHandle_t s_tacheHp = nullptr, s_tacheMicro = nullptr;

bool voixActive() { return s_active; }
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

static void tacheHautParleur(void *) {
  static int16_t bloc[128];
  for (;;) {
    if (!s_active) {
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      // Petite réserve de 40 ms avant de jouer, pour lisser l'arrivée des paquets
      size_t reserve = s_frequence * 2 / 25;
      for (int i = 0; i < 20 && s_active; i++) {
        UBaseType_t libre = xRingbufferGetCurFreeSize(s_versHp);
        if (4096 - libre >= reserve) break;
        vTaskDelay(pdMS_TO_TICKS(10));
      }
      continue;
    }
    size_t n = 0;
    uint8_t *p = (uint8_t *)xRingbufferReceiveUpTo(s_versHp, &n, pdMS_TO_TICKS(50), sizeof bloc);
    if (!p) continue;
    memcpy(bloc, p, n);
    vRingbufferReturnItem(s_versHp, p);
    audioEcrireMono(bloc, n / 2, s_gainQ15, ModeAudio::APPEL);
  }
}

// ---------- micro -> téléphone ----------

uint32_t voixAEnvoyer(uint8_t *donnees, uint32_t taille) {
  uint32_t fait = 0;
  while (fait < taille) {
    size_t n = 0;
    uint8_t *p = (uint8_t *)xRingbufferReceiveUpTo(s_versTel, &n, 0, taille - fait);
    if (!p) break;
    memcpy(donnees + fait, p, n);
    vRingbufferReturnItem(s_versTel, p);
    fait += n;
  }
  if (fait < taille) memset(donnees + fait, 0, taille - fait);   // pas assez de son : du silence
  return taille;
}

#if AVEC_MICRO
static bool microOuvrir() {
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_RX);
  cfg.sample_rate = FREQ_MICRO;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT;   // l'INMP441 envoie 24 bits dans 32
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 4;
  cfg.dma_buf_len = BLOC_MICRO;
  cfg.use_apll = false;
  if (i2s_driver_install(PORT_MICRO, &cfg, 0, nullptr) != ESP_OK) {
    Serial.println("[Voix] Impossible de démarrer le micro (I2S1)");
    return false;
  }
  i2s_pin_config_t broches = {};
  broches.mck_io_num = I2S_PIN_NO_CHANGE;
  broches.bck_io_num = BROCHE_MICRO_SCK;
  broches.ws_io_num = BROCHE_MICRO_WS;
  broches.data_out_num = I2S_PIN_NO_CHANGE;
  broches.data_in_num = BROCHE_MICRO_SD;
  i2s_set_pin(PORT_MICRO, &broches);
  return true;
}

static void tacheMicro(void *) {
  static int32_t brut[2 * BLOC_MICRO];
  static int16_t sortie[BLOC_MICRO];
  bool ouvert = false;
  int canal = MICRO_CANAL;
  uint64_t energie[2] = {0, 0};
  int blocsMesures = 0;
  int32_t dcEntree = 0, dcSortie = 0;   // filtre passe-haut : retire la composante continue

  for (;;) {
    if (!s_active) {
      if (ouvert) {
        i2s_driver_uninstall(PORT_MICRO);
        ouvert = false;
      }
      ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
      continue;
    }
    if (!ouvert) {
      if (!microOuvrir()) { s_active = false; continue; }
      ouvert = true;
      canal = MICRO_CANAL;
      energie[0] = energie[1] = 0;
      blocsMesures = 0;
      dcEntree = dcSortie = 0;
    }
    size_t lus = 0;
    i2s_read(PORT_MICRO, brut, sizeof brut, &lus, pdMS_TO_TICKS(50));
    size_t trames = lus / 8;
    if (trames == 0) continue;

    // Le micro (L/R à GND) parle sur une seule des deux voies : on garde la plus vivante.
    if (canal < 0) {
      for (size_t i = 1; i < trames; i++)
        for (int c = 0; c < 2; c++) {
          int32_t d = (brut[2 * i + c] >> 14) - (brut[2 * i - 2 + c] >> 14);
          energie[c] += (uint64_t)((int64_t)d * d);
        }
      if (++blocsMesures >= 30) {
        canal = energie[1] > energie[0] ? 1 : 0;
        Serial.printf("[Voix] Micro détecté sur la voie %d\n", canal);
      }
    }
    int c = canal < 0 ? 0 : canal;
    size_t n = 0;
    for (size_t i = 0; i < trames; i++) {
      int32_t x = (brut[2 * i + c] >> 14) * GAIN_MICRO / 4;   // 18 bits utiles -> 16 bits environ
      int32_t y = x - dcEntree + dcSortie - (dcSortie >> 8);
      dcEntree = x;
      dcSortie = y;
      if (y > 32767) y = 32767;
      if (y < -32768) y = -32768;
      sortie[n++] = (int16_t)y;
    }
    if (s_frequence == 8000) {   // 16 kHz -> 8 kHz : moyenne de deux échantillons
      size_t m = 0;
      for (size_t i = 0; i + 1 < n; i += 2) sortie[m++] = (int16_t)(((int32_t)sortie[i] + sortie[i + 1]) / 2);
      n = m;
    }
    xRingbufferSend(s_versTel, sortie, n * 2, 0);   // tampon plein : on perd ce bloc
  }
}
#endif

// ---------- démarrage / arrêt ----------

void voixInit() {
  s_versHp = xRingbufferCreate(4096, RINGBUF_TYPE_BYTEBUF);
  s_versTel = xRingbufferCreate(2048, RINGBUF_TYPE_BYTEBUF);
  voixSetVolume(VOLUME_APPEL_DEPART);
  xTaskCreate(tacheHautParleur, "voix_hp", VOIX_PILE, nullptr, VOIX_PRIO, &s_tacheHp);
#if AVEC_MICRO
  xTaskCreate(tacheMicro, "voix_micro", VOIX_PILE, nullptr, VOIX_PRIO, &s_tacheMicro);
#endif
}

void voixDemarrer(uint32_t frequence) {
  vider(s_versHp);
  vider(s_versTel);
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
