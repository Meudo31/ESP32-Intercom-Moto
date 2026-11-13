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
// Le bouton A est toujours présent. Combinaisons d'appuis :
//   court = lecture / pause         court + court = morceau suivant
//   court + long = morceau précédent  long = volume +    long + court = volume -
//   très long (maintenu) = veille profonde ; un appui la réveille
#define GESTE_LONG_MS       600    // un appui plus long que ça est « long »
#define GESTE_TRES_LONG_MS 3000    // durée de l'appui « très long » (veille)
#define GESTE_ATTENTE_MS    400    // délai pour enchaîner un second appui
#define PAS_VOLUME_BOUTON    10    // pas de volume du téléphone, sur 127 (10 = environ 8 %)
// Boutons B et C : 0 = absents, 1 = sur GPIO33 et 27 (volume et pistes).
#define AVEC_BOUTONS 0
// Veille automatique après N minutes sans téléphone ni app connectés, 0 = jamais
// (valeur de départ, modifiable depuis l'app).
#define VEILLE_AUTO_MIN 0
// Clé demandée par l'app pour mettre à jour le firmware par Bluetooth
#define OTA_KEY "intercom-moto"

// Mise à jour par Wi-Fi : durée d'ouverture du point d'accès (secondes)
#define WIFI_OTA_DUREE_S 300

// ---------- Broches ----------
// (GPIO12 est à éviter : s'il est tiré vers le haut au démarrage, l'ESP32 ne démarre plus)
#define BROCHE_I2S_BCLK  27   // MAX98357A : BCLK
#define BROCHE_I2S_LRC   26   // MAX98357A : LRC
#define BROCHE_I2S_DOUT  14   // MAX98357A : DIN
#define BROCHE_AMPLI_SD  13   // MAX98357A : SD (coupe l'ampli en veille), -1 si non câblée
#define BROCHE_LED        2   // LED bleue de la carte
#define BROCHE_BOUTON_A  15   // doit être une broche RTC (réveil de la veille profonde)
#define BROCHE_BOUTON_B  33
#define BROCHE_BOUTON_C   4

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
#define WIFI_OTA_PRIO     2
#define WIFI_OTA_PILE     6144

// Période d'envoi des données à l'app (ms)
#define BLE_NOTIFY_PERIOD_MS 250
