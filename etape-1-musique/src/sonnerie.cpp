#include "sonnerie.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "audio.h"
#include "parametres.h"
#include "sonnerie_wav.h"   // généré depuis data/sonnerie.wav par outils/sonnerie_wav.py

static const uint8_t *s_donnees = nullptr;   // échantillons du fichier
static uint32_t s_taille = 0;                // en octets
static uint32_t s_frequence = 16000;
static uint16_t s_voies = 1, s_bits = 16;
static volatile bool s_active = false;
static TaskHandle_t s_tache = nullptr;

static uint32_t lire32(const uint8_t *p) { return p[0] | (p[1] << 8) | (p[2] << 16) | ((uint32_t)p[3] << 24); }
static uint16_t lire16(const uint8_t *p) { return p[0] | (p[1] << 8); }

// Cherche les blocs « fmt » et « data » du fichier WAV.
static bool lireWav(const uint8_t *f, size_t n) {
  if (n < 12 || memcmp(f, "RIFF", 4) || memcmp(f + 8, "WAVE", 4)) return false;
  bool fmt = false;
  size_t i = 12;
  while (i + 8 <= n) {
    uint32_t taille = lire32(f + i + 4);
    const uint8_t *bloc = f + i + 8;
    if (!memcmp(f + i, "fmt ", 4) && taille >= 16) {
      if (lire16(bloc) != 1) return false;   // 1 = PCM non compressé
      s_voies = lire16(bloc + 2);
      s_frequence = lire32(bloc + 4);
      s_bits = lire16(bloc + 14);
      fmt = true;
    } else if (!memcmp(f + i, "data", 4)) {
      s_donnees = bloc;
      s_taille = min<uint32_t>(taille, n - (i + 8));
    }
    i += 8 + taille + (taille & 1);
  }
  return fmt && s_donnees && (s_voies == 1 || s_voies == 2) && (s_bits == 8 || s_bits == 16) &&
         s_frequence >= 8000 && s_frequence <= 48000;
}

// Échantillon mono 16 bits numéro i du fichier.
static int16_t echantillon(uint32_t i) {
  if (s_bits == 16) {   // lu octet par octet : le tableau n'est pas forcément aligné
    const uint8_t *e = s_donnees + i * s_voies * 2;
    int32_t g = (int16_t)lire16(e);
    return s_voies == 2 ? (int16_t)((g + (int16_t)lire16(e + 2)) / 2) : (int16_t)g;
  }
  const uint8_t *e = s_donnees + i * s_voies;   // 8 bits : non signé, 128 = silence
  int32_t v = s_voies == 2 ? ((int32_t)e[0] + e[1]) / 2 : e[0];
  return (int16_t)((v - 128) << 8);
}

static void tacheSonnerie(void *) {
  int16_t bloc[128];
  const int32_t gain = (int32_t)SONNERIE_VOLUME * 32768 / 100;
  for (;;) {
    ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
    while (s_active) {
      uint32_t total = s_taille / (s_voies * s_bits / 8);
      for (uint32_t i = 0; i < total && s_active;) {
        uint32_t k = min<uint32_t>(128, total - i);
        for (uint32_t j = 0; j < k; j++) bloc[j] = echantillon(i + j);
        if (audioMode() != ModeAudio::SONNERIE) break;   // appel décroché ou rejeté
        audioEcrireMono(bloc, k, gain, ModeAudio::SONNERIE);
        i += k;
      }
      for (uint32_t t = 0; t < SONNERIE_PAUSE_MS && s_active; t += 20) vTaskDelay(pdMS_TO_TICKS(20));
    }
  }
}

void sonnerieInit() {
  if (!lireWav(SONNERIE_WAV, SONNERIE_WAV_TAILLE)) {
    Serial.println("[Sonnerie] data/sonnerie.wav illisible (WAV PCM 8 ou 16 bits attendu) : pas de sonnerie");
    s_donnees = nullptr;
    s_taille = 0;
  } else {
    Serial.printf("[Sonnerie] %u Hz, %u bits, %u voie(s), %.1f s\n", (unsigned)s_frequence, s_bits, s_voies,
                  s_taille / (float)(s_frequence * s_voies * s_bits / 8));
  }
  xTaskCreate(tacheSonnerie, "sonnerie", SONNERIE_PILE, nullptr, SONNERIE_PRIO, &s_tache);
}

void sonnerieDemarrer() {
  if (s_active || !s_donnees) return;
  s_active = true;
  xTaskNotifyGive(s_tache);
}

void sonnerieArreter() { s_active = false; }
bool sonnerieActive() { return s_active; }
bool sonnerieDisponible() { return s_donnees != nullptr; }
uint32_t sonnerieFrequence() { return s_frequence; }
