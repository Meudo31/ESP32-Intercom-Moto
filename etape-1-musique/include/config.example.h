// Copier ce fichier en config.h et adapter les valeurs.
// Tous les réglages du firmware sont ici. Un réglage absent de config.h prend la valeur
// par défaut de parametres.h, donc un ancien config.h continue de compiler.
#pragma once

// ---------- Général ----------
// Nom visible dans le Bluetooth du téléphone (modifiable ensuite depuis l'app)
#define NOM_BLUETOOTH "Intercom Moto"
// Volume de l'intercom au premier démarrage, de 0 à 100 % (ensuite mémorisé).
// Le volume du téléphone s'applique en plus.
#define VOLUME_DEPART 100
// Le bouton A (veille profonde, lecture / pause) est toujours présent.
// Boutons B et C : 0 = absents, 1 = sur GPIO33 et 27 (volume et pistes).
#define AVEC_BOUTONS 0
// Veille automatique après N minutes sans téléphone ni app connectés, 0 = jamais
// (valeur de départ, modifiable depuis l'app).
#define VEILLE_AUTO_MIN 0
// Clé demandée par l'app pour mettre à jour le firmware par Bluetooth
#define OTA_KEY "intercom-moto"

// ---------- Broches ----------
#define BROCHE_I2S_BCLK  26   // MAX98357A : BCLK
#define BROCHE_I2S_LRC   25   // MAX98357A : LRC
#define BROCHE_I2S_DOUT  22   // MAX98357A : DIN
#define BROCHE_LED        2   // LED bleue de la carte
#define BROCHE_BOUTON_A  32   // doit être une broche RTC (réveil de la veille profonde)
#define BROCHE_BOUTON_B  33
#define BROCHE_BOUTON_C  27

// ---------- Tâches FreeRTOS (priorité, taille de pile en octets) ----------
#define COMMANDES_PRIO    3
#define COMMANDES_PILE    4096
#define BOUTONS_PRIO      2
#define BOUTONS_PILE      3072
#define BLE_TASK_PRIO     2
#define BLE_TASK_STACK    4096
#define SURVEILLANCE_PRIO 1
#define SURVEILLANCE_PILE 3072
#define LED_PRIO          1
#define LED_PILE          2048

// Période d'envoi des données à l'app (ms)
#define BLE_NOTIFY_PERIOD_MS 250
