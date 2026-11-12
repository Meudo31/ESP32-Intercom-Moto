// Intercom moto - étape 1 : lecteur de musique Bluetooth dans le casque
//
// Fonctionnement :
//  1. L'ESP32 apparaît comme une enceinte Bluetooth (« Intercom Moto ») sur le téléphone.
//  2. La musique reçue (A2DP) est envoyée en numérique (I2S) à l'ampli MAX98357A,
//     qui alimente les haut-parleurs du casque.
//  3. Un bouton (A, GPIO32) : appui long de 2 s = veille profonde, un appui le réveille ;
//     appui court = lecture / pause.
//  4. En option (AVEC_BOUTONS à 1 dans config.h), deux boutons de plus pilotent le volume
//     et les pistes. Sans eux, tout se règle depuis le téléphone.
//
// Carte conseillée : D1 mini ESP32 (WROOM-32). Toute carte ESP32 classique convient.
//
// Boutons (entre la broche et GND) :
//   A : court = lecture / pause, long (2 s) = veille profonde   (toujours présent)
//   B et C seulement si AVEC_BOUTONS vaut 1 :
//   B : court = volume +, long = piste suivante
//   C : court = volume -, long = piste précédente
//
// Attention : il faut un ESP32 « classique » (ESP32-WROOM-32). Les ESP32-S3 et C3
// n'ont pas de Bluetooth classique et ne peuvent pas recevoir de musique.

#include <Arduino.h>
#include "BluetoothA2DPSink.h"
#include "driver/i2s.h"
#include "driver/rtc_io.h"
#include "bouton.h"
#include "config.h"

#ifndef AVEC_BOUTONS
#define AVEC_BOUTONS 0
#endif

// ---------- Broches ----------
#define BROCHE_I2S_BCLK  26   // MAX98357A : BCLK
#define BROCHE_I2S_LRC   25   // MAX98357A : LRC
#define BROCHE_I2S_DOUT  22   // MAX98357A : DIN
#define BROCHE_LED        2   // LED bleue de la carte : allumée quand le téléphone est connecté
#define BROCHE_VEILLE    32   // bouton A : doit être une broche RTC pour réveiller l'ESP32
#define PORT_HP          I2S_NUM_0

Bouton boutonA(BROCHE_VEILLE, 2000);  // appui long de 2 s pour éviter une mise en veille par erreur
#if AVEC_BOUTONS
Bouton boutonB(33), boutonC(27);
#endif
BluetoothA2DPSink a2dp;

volatile int volume = VOLUME_DEPART;    // 0 à 100 %
volatile int32_t gainQ15 = 0;           // gain appliqué aux échantillons (32768 = 1,0)

// ---------- Volume ----------
// Courbe quadratique : l'oreille perçoit mieux les petits volumes ainsi.
void appliquerVolume(int nouveau) {
  volume = constrain(nouveau, 0, 100);
  gainQ15 = (int32_t)volume * volume * 32768 / 10000;
  Serial.printf("Volume : %d %%\n", volume);
}

// ---------- Sortie audio ----------
void initHautParleur() {
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
}

// Appelée par la pile Bluetooth à chaque paquet de musique (16 bits stéréo entrelacé).
// Le volume du téléphone est déjà appliqué ; on ajoute le volume local puis on envoie à l'ampli.
void recevoirMusique(const uint8_t* donnees, uint32_t taille) {
  static int16_t tampon[512];
  const int16_t* ech = (const int16_t*)donnees;
  uint32_t total = taille / 2;
  int32_t g = gainQ15;
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
void changerFrequence(uint16_t frequence) {
  Serial.printf("Fréquence audio : %u Hz\n", frequence);
  i2s_set_sample_rates(PORT_HP, frequence);
}

void connexionChangee(esp_a2d_connection_state_t etat, void*) {
  bool connecte = etat == ESP_A2D_CONNECTION_STATE_CONNECTED;
  digitalWrite(BROCHE_LED, connecte);
  Serial.println(connecte ? "Téléphone connecté" : "Téléphone déconnecté");
}

// ---------- Veille profonde ----------
// L'ESP32 coupe presque tout (quelques µA). Un appui sur le bouton A le réveille :
// il redémarre alors comme à la mise sous tension et se reconnecte au téléphone.
void attendreRelachement() {
  while (digitalRead(BROCHE_VEILLE) == LOW) delay(10);
  delay(50);  // anti-rebond
}

void veilleProfonde() {
  Serial.println("Veille profonde : appuie sur le bouton A pour réveiller");
  digitalWrite(BROCHE_LED, LOW);
  attendreRelachement();          // sinon le bouton encore enfoncé réveillerait aussitôt
  a2dp.end();                     // déconnexion propre du téléphone
  i2s_driver_uninstall(PORT_HP);
  rtc_gpio_pullup_en((gpio_num_t)BROCHE_VEILLE);
  rtc_gpio_pulldown_dis((gpio_num_t)BROCHE_VEILLE);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BROCHE_VEILLE, 0);  // réveil quand la broche passe à GND
  Serial.flush();
  esp_deep_sleep_start();
}

// ---------- Programme principal ----------
void setup() {
  Serial.begin(115200);
  Serial.println("\nIntercom moto - étape 1 : musique Bluetooth");
  pinMode(BROCHE_LED, OUTPUT);
  boutonA.begin();
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("Réveil par le bouton");
    attendreRelachement();        // l'appui de réveil ne doit pas compter comme un appui long
  }
#if AVEC_BOUTONS
  boutonB.begin();
  boutonC.begin();
#endif
  // Sans boutons, le volume local reste au maximum : c'est le téléphone qui règle le volume.
  appliquerVolume(AVEC_BOUTONS ? VOLUME_DEPART : 100);

  initHautParleur();
  a2dp.set_stream_reader(recevoirMusique, false);  // false : on gère l'I2S nous-mêmes
  a2dp.set_sample_rate_callback(changerFrequence);
  a2dp.set_on_connection_state_changed(connexionChangee);
  a2dp.start(NOM_BLUETOOTH);  // reconnexion automatique au dernier téléphone
  Serial.printf("Prêt : cherche « %s » dans le Bluetooth du téléphone\n", NOM_BLUETOOTH);
}

void loop() {
  switch (boutonA.lire()) {
    case Appui::COURT:
      if (a2dp.get_audio_state() == ESP_A2D_AUDIO_STATE_STARTED) {
        Serial.println("Pause");
        a2dp.pause();
      } else {
        Serial.println("Lecture");
        a2dp.play();
      }
      break;
    case Appui::LONG: veilleProfonde(); break;
    default: break;
  }
#if AVEC_BOUTONS
  switch (boutonB.lire()) {
    case Appui::COURT: appliquerVolume(volume + 10); break;
    case Appui::LONG:  Serial.println("Piste suivante"); a2dp.next(); break;
    default: break;
  }
  switch (boutonC.lire()) {
    case Appui::COURT: appliquerVolume(volume - 10); break;
    case Appui::LONG:  Serial.println("Piste précédente"); a2dp.previous(); break;
    default: break;
  }
#endif
  delay(5);
}
