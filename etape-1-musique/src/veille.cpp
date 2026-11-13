#include "veille.h"

#include <Arduino.h>
#include "driver/rtc_io.h"
#include "audio.h"
#include "musique_bt.h"
#include "ble_ota.h"
#include "parametres.h"
#include "reglages.h"
#include "system_events.h"

static void attendreRelachement() {
  while (digitalRead(BROCHE_BOUTON_A) == LOW) delay(10);
  delay(50);  // anti-rebond
}

void veilleAuReveil() {
  pinMode(BROCHE_BOUTON_A, INPUT_PULLUP);
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0) {
    Serial.println("[Veille] Réveil par le bouton");
    attendreRelachement();        // l'appui de réveil ne doit pas compter comme un appui long
  }
}

void veilleProfonde() {
  Serial.println("[Veille] Veille profonde : appuie sur le bouton A pour réveiller");
  digitalWrite(BROCHE_LED, LOW);
  attendreRelachement();          // sinon le bouton encore enfoncé réveillerait aussitôt
  delay(300);                     // laisse partir la réponse à l'app
  musiqueArreter();               // déconnexion propre du téléphone
  audioArreter();
  ampliCouperPourVeille();         // l'ampli ne consomme plus rien
  rtc_gpio_pullup_en((gpio_num_t)BROCHE_BOUTON_A);
  rtc_gpio_pulldown_dis((gpio_num_t)BROCHE_BOUTON_A);
  esp_sleep_enable_ext0_wakeup((gpio_num_t)BROCHE_BOUTON_A, 0);  // réveil quand la broche passe à GND
  Serial.flush();
  esp_deep_sleep_start();
}

static void tacheSurveillance(void *) {
  uint32_t seulDepuis = millis();
  for (;;) {
    musiqueRafraichirSignal();
    bool occupe = sysGet() & (SYS_TEL_CONNECTE | SYS_HFP_CONNECTE | SYS_BLE_CONNECTED | SYS_OTA);
    if (occupe) seulDepuis = millis();
    uint8_t minutes = reglagesVeilleAuto();
    if (minutes > 0 && millis() - seulDepuis > minutes * 60000UL) {
      Serial.printf("[Veille] Aucune connexion depuis %u min\n", minutes);
      veilleProfonde();
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void surveillanceDemarrer() {
  xTaskCreate(tacheSurveillance, "surveillance", SURVEILLANCE_PILE, nullptr, SURVEILLANCE_PRIO, nullptr);
}
