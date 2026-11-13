#include "audio.h"
#include <Arduino.h>
#include "driver/gpio.h"
#include "driver/i2s.h"
#include "parametres.h"

#define PORT_HP I2S_NUM_0

static volatile uint8_t s_volume = 100;
static volatile int32_t s_gainQ15 = 32768;    // 32768 = 1,0
static volatile uint32_t s_octets = 0;
static bool s_actif = false;
static volatile ModeAudio s_mode = ModeAudio::MUSIQUE;
static volatile uint16_t s_freqMusique = 44100;

void audioInit() {
  ampliAllumer();
  i2s_config_t cfg = {};
  cfg.mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX);
  cfg.sample_rate = 44100;
  cfg.bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT;
  cfg.channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags = 0;
  cfg.dma_buf_count = 8;
  cfg.dma_buf_len = 256;
  cfg.use_apll = false;
  cfg.tx_desc_auto_clear = true;  // silence automatique si plus rien n'arrive
  i2s_driver_install(PORT_HP, &cfg, 0, nullptr);

  i2s_pin_config_t broches = {};
  broches.mck_io_num = I2S_PIN_NO_CHANGE;
  broches.bck_io_num = BROCHE_I2S_BCLK;
  broches.ws_io_num = BROCHE_I2S_LRC;
  broches.data_out_num = BROCHE_I2S_DOUT;
  broches.data_in_num = I2S_PIN_NO_CHANGE;
  i2s_set_pin(PORT_HP, &broches);
  s_actif = true;
}

// SD laissée en l'air (haute impédance) : la résistance du module la tire vers le haut,
// ce qui allume l'ampli en mono (gauche + droite). SD à GND : ampli coupé.
void ampliAllumer() {
  if (BROCHE_AMPLI_SD < 0) return;
  gpio_hold_dis((gpio_num_t)BROCHE_AMPLI_SD);   // maintien posé avant la veille profonde
  gpio_deep_sleep_hold_dis();
  pinMode(BROCHE_AMPLI_SD, INPUT);
}

// GAIN du MAX98357A : à GND = 12 dB, en l'air = 9 dB, au + = 6 dB.
// L'ampli lit GAIN en sortant de l'arrêt : on le coupe un instant par SD pour qu'il relise.
static uint8_t s_gain = 9;

uint8_t ampliGain() { return s_gain; }

uint8_t ampliSetGain(uint8_t db) {
  s_gain = db >= 11 ? 12 : (db <= 7 ? 6 : 9);
  if (BROCHE_AMPLI_GAIN < 0) return s_gain;
  if (s_gain == 12) {
    pinMode(BROCHE_AMPLI_GAIN, OUTPUT);
    digitalWrite(BROCHE_AMPLI_GAIN, LOW);
  } else if (s_gain == 6) {
    pinMode(BROCHE_AMPLI_GAIN, OUTPUT);
    digitalWrite(BROCHE_AMPLI_GAIN, HIGH);
  } else {
    pinMode(BROCHE_AMPLI_GAIN, INPUT);
  }
  if (BROCHE_AMPLI_SD >= 0) {
    pinMode(BROCHE_AMPLI_SD, OUTPUT);
    digitalWrite(BROCHE_AMPLI_SD, LOW);
    delay(20);
    pinMode(BROCHE_AMPLI_SD, INPUT);   // rallume l'ampli, qui relit GAIN
  }
  Serial.printf("[Audio] Gain de l'ampli : %u dB\n", s_gain);
  return s_gain;
}

void ampliCouperPourVeille() {
  if (BROCHE_AMPLI_SD < 0) return;
  pinMode(BROCHE_AMPLI_SD, OUTPUT);
  digitalWrite(BROCHE_AMPLI_SD, LOW);
  gpio_hold_en((gpio_num_t)BROCHE_AMPLI_SD);     // reste à GND pendant la veille
  gpio_deep_sleep_hold_en();
}

void audioArreter() {
  if (!s_actif) return;
  s_actif = false;
  i2s_driver_uninstall(PORT_HP);
}

// Courbe quadratique : l'oreille perçoit mieux les petits volumes ainsi.
void audioSetVolume(int volume) {
  volume = constrain(volume, 0, 100);
  s_volume = volume;
  s_gainQ15 = (int32_t)volume * volume * 32768 / 10000;
}

uint8_t audioVolume() { return s_volume; }
uint32_t audioOctetsRecus() { return s_octets; }

// Le volume du téléphone est déjà appliqué ; on ajoute le volume local puis on envoie à l'ampli.
void audioRecevoir(const uint8_t *donnees, uint32_t taille) {
  static int16_t tampon[512];
  s_octets += taille;
  if (!s_actif || s_mode != ModeAudio::MUSIQUE) return;   // sonnerie ou appel en cours
  const int16_t *ech = (const int16_t *)donnees;
  uint32_t total = taille / 2;
  int32_t g = s_gainQ15;
  while (total > 0) {
    uint32_t n = min<uint32_t>(total, 512);
    for (uint32_t i = 0; i < n; i++) tampon[i] = (int16_t)(((int32_t)ech[i] * g) >> 15);
    size_t ecrits;
    i2s_write(PORT_HP, tampon, n * 2, &ecrits, portMAX_DELAY);
    ech += n;
    total -= n;
  }
}

// La plupart des téléphones envoient du 44,1 kHz, certains du 48 kHz.
void audioFrequence(uint16_t frequence) {
  Serial.printf("[Audio] Fréquence : %u Hz\n", frequence);
  s_freqMusique = frequence;
  if (s_actif && s_mode == ModeAudio::MUSIQUE) i2s_set_sample_rates(PORT_HP, frequence);
}

ModeAudio audioMode() { return s_mode; }

void audioSetMode(ModeAudio mode, uint32_t frequence) {
  static uint32_t s_freqMode = 0;
  uint32_t f = mode == ModeAudio::MUSIQUE ? s_freqMusique : frequence;
  if (mode == s_mode && (mode == ModeAudio::MUSIQUE || f == s_freqMode)) return;
  s_mode = mode;
  s_freqMode = f;
  if (!s_actif) return;
  i2s_zero_dma_buffer(PORT_HP);
  i2s_set_sample_rates(PORT_HP, f);
  static const char *noms[] = {"musique", "sonnerie", "appel"};
  Serial.printf("[Audio] Source : %s, %u Hz\n", noms[(int)mode], (unsigned)f);
}

void audioEcrireMono(const int16_t *ech, size_t n, int32_t gainQ15, ModeAudio mode) {
  static int16_t tampon[2 * 128];   // utilisé par une seule source à la fois
  if (!s_actif || s_mode != mode) return;
  int32_t g = (int32_t)(((int64_t)s_gainQ15 * gainQ15) >> 15);
  while (n > 0) {
    size_t k = n < 128 ? n : 128;
    for (size_t i = 0; i < k; i++) {
      int32_t v = ((int32_t)ech[i] * g) >> 15;
      if (v > 32767) v = 32767;
      if (v < -32768) v = -32768;
      tampon[2 * i] = tampon[2 * i + 1] = (int16_t)v;
    }
    size_t ecrits;
    i2s_write(PORT_HP, tampon, k * 4, &ecrits, portMAX_DELAY);
    ech += k;
    n -= k;
    if (s_mode != mode) return;
  }
}
