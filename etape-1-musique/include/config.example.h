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
// Boutons B et C : 0 = absents, 1 = sur GPIO16 et 17 (volume et pistes).
#define AVEC_BOUTONS 0
// Veille automatique après N minutes sans téléphone ni app connectés, 0 = jamais
// (valeur de départ, modifiable depuis l'app).
#define VEILLE_AUTO_MIN 0
// Gain de l'ampli au premier démarrage : 6, 9 ou 12 dB (ensuite réglable depuis l'app et mémorisé)
#define GAIN_DEPART 9
// Clé demandée par l'app pour mettre à jour le firmware par Bluetooth
#define OTA_KEY "intercom-moto"

// Mise à jour par Wi-Fi : durée d'ouverture du point d'accès (secondes)
#define WIFI_OTA_DUREE_S 300

// ---------- Appels téléphoniques (profil Bluetooth « mains libres », HFP) ----------
// Pendant un appel entrant, la sonnerie data/sonnerie.wav tourne en boucle dans le casque.
//   sonnerie : court = décrocher, court + court = rejeter
//   en appel : court = raccrocher, long = volume +, long + court = volume -
// Micro I2S INMP441 : 1 = câblé (voir README), 0 = absent (l'interlocuteur n'entend rien)
#define AVEC_MICRO 1
#define GAIN_MICRO 4            // amplification du micro, de 1 à 32
#define MICRO_CANAL -1          // -1 = détection automatique, 0 ou 1 pour forcer
#define SONNERIE_VOLUME 80      // volume de la sonnerie, en % du volume de l'intercom
#define SONNERIE_PAUSE_MS 1500  // silence entre deux sonneries
#define VOLUME_APPEL_DEPART 10  // volume de l'appel (0 à 15) tant que le téléphone ne l'a pas donné

// ---------- Broches ----------
#define BROCHE_I2S_BCLK  27   // MAX98357A : BCLK
#define BROCHE_I2S_LRC   26   // MAX98357A : LRC
#define BROCHE_I2S_DOUT  14   // MAX98357A : DIN
#define BROCHE_AMPLI_SD  13   // MAX98357A : SD (coupe l'ampli en veille), -1 si non câblée
#define BROCHE_AMPLI_GAIN 12  // MAX98357A : GAIN (6, 9 ou 12 dB), -1 si non câblée
                              // GPIO12 = broche de démarrage : voir README (« Gain sur GPIO12 »)
#define BROCHE_LED       -1   // LED bleue de la carte (GPIO2), -1 : GPIO2 sert de masse au bouton
#define BROCHE_BOUTON_A   4   // doit être une broche RTC (réveil de la veille profonde)
#define BROCHE_BOUTON_A_MASSE 2  // broche mise à GND par le firmware (autre patte du bouton), -1 si le bouton va à GND
#define BROCHE_BOUTON_B  16
#define BROCHE_BOUTON_C  17
#define BROCHE_MICRO_SCK 25   // INMP441 : SCK
#define BROCHE_MICRO_WS  32   // INMP441 : WS
#define BROCHE_MICRO_SD  35   // INMP441 : SD (GPIO35 = entrée seulement, idéal ici)
#define BROCHE_MICRO_LR  33   // INMP441 : L/R, mise à GND par le firmware (voie gauche), -1 si L/R câblée à GND

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
#define APPEL_PRIO        3
#define APPEL_PILE        4096
#define SONNERIE_PRIO     4
#define SONNERIE_PILE     3072
#define VOIX_PRIO         5     // voix d'appel : haut-parleur et micro
#define VOIX_PILE         3072

// Période d'envoi des données à l'app (ms)
#define BLE_NOTIFY_PERIOD_MS 250
