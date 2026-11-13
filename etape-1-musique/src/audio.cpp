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
  if (!s_actif) return;
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
  if (s_actif) i2s_set_sample_rates(PORT_HP, frequence);
}
