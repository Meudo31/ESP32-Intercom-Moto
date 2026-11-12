// Intercom moto - étape 1 : lecteur de musique Bluetooth dans le casque
//
//  1. L'ESP32 apparaît comme une enceinte Bluetooth (« Intercom Moto ») sur le téléphone.
//  2. La musique reçue (A2DP) part en I2S vers l'ampli MAX98357A et les haut-parleurs du casque.
//  3. Bouton A (GPIO32) : court = lecture / pause, court + court = suivant,
//     court + long = précédent, long = volume +, long + court = volume -,
//     très long (3 s) = veille profonde, un appui la réveille.
//     Boutons B et C en option (AVEC_BOUTONS dans config.h).
//  4. L'app Android « Intercom Moto » se connecte en Bluetooth basse consommation (BLE) :
//     elle affiche toutes les infos, envoie les commandes et met à jour le firmware.
//
// Organisation FreeRTOS (un module .h/.cpp par élément) :
//   musique_bt   pile Bluetooth classique (ses propres tâches) -> audio (I2S)
//   commandes    tâche qui exécute les commandes (file d'attente) des boutons et de l'app
//   boutons_task tâche qui lit les boutons
//   ble_task     tâche BLE : données 4 fois par seconde, commandes, mise à jour (ble_ota)
//   veille       tâche de surveillance : signal Bluetooth, veille automatique
//   led_task     tâche LED
//   etat (mutex) et system_events (EventGroup) relient le tout.
//
// Attention : il faut un ESP32 « classique » (WROOM-32, par exemple la D1 mini ESP32).
// Les ESP32-S3, C3, C6 et P4 n'ont pas de Bluetooth classique et ne peuvent pas recevoir de musique.

#include <Arduino.h>
#include "audio.h"
#include "ble_task.h"
#include "boutons_task.h"
#include "commandes.h"
#include "etat.h"
#include "led_task.h"
#include "musique_bt.h"
#include "parametres.h"
#include "reglages.h"
#include "system_events.h"
#include "veille.h"

void setup() {
  Serial.begin(115200);
  Serial.printf("\nIntercom moto - étape 1 : musique Bluetooth, firmware %s\n", FW_VERSION);
  sysInit();
  etatInit();
  veilleAuReveil();
  reglagesCharger();

  audioInit();
  audioSetVolume(reglagesVolume());
  musiqueDemarrer(reglagesNom());

  commandesDemarrer();
  boutonsDemarrer();
  ledDemarrer();
  surveillanceDemarrer();
  bleTaskStart();
}

void loop() {
  vTaskDelete(nullptr);   // tout se passe dans les tâches
}
