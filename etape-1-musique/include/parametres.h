// Réglages : config.h d'abord, puis valeurs par défaut pour ce qui n'y est pas.
#pragma once
#include "config.h"
#include "version.h"

#ifndef NOM_BLUETOOTH
#define NOM_BLUETOOTH "Intercom Moto"
#endif
#ifndef VOLUME_DEPART
#define VOLUME_DEPART 100
#endif
#ifndef AVEC_BOUTONS
#define AVEC_BOUTONS 0
#endif
#ifndef VEILLE_AUTO_MIN
#define VEILLE_AUTO_MIN 0
#endif
#ifndef OTA_KEY
#define OTA_KEY "intercom-moto"
#endif
#ifndef BROCHE_I2S_BCLK
#define BROCHE_I2S_BCLK 27
#endif
#ifndef BROCHE_I2S_LRC
#define BROCHE_I2S_LRC 26
#endif
#ifndef BROCHE_I2S_DOUT
#define BROCHE_I2S_DOUT 14
#endif
#ifndef BROCHE_AMPLI_SD
#define BROCHE_AMPLI_SD 13
#endif
#ifndef BROCHE_AMPLI_GAIN
#define BROCHE_AMPLI_GAIN 25
#endif
#ifndef GAIN_DEPART
#define GAIN_DEPART 9
#endif
#ifndef BROCHE_LED
#define BROCHE_LED 2
#endif
#ifndef BROCHE_BOUTON_A
#define BROCHE_BOUTON_A 15
#endif
#ifndef BROCHE_BOUTON_B
#define BROCHE_BOUTON_B 33
#endif
#ifndef BROCHE_BOUTON_C
#define BROCHE_BOUTON_C 4
#endif
#ifndef COMMANDES_PRIO
#define COMMANDES_PRIO 3
#endif
#ifndef COMMANDES_PILE
#define COMMANDES_PILE 4096
#endif
#ifndef BOUTONS_PRIO
#define BOUTONS_PRIO 2
#endif
#ifndef BOUTONS_PILE
#define BOUTONS_PILE 3072
#endif
#ifndef BLE_TASK_PRIO
#define BLE_TASK_PRIO 2
#endif
#ifndef BLE_TASK_STACK
#define BLE_TASK_STACK 4096
#endif
#ifndef SURVEILLANCE_PRIO
#define SURVEILLANCE_PRIO 1
#endif
#ifndef SURVEILLANCE_PILE
#define SURVEILLANCE_PILE 3072
#endif
#ifndef LED_PRIO
#define LED_PRIO 1
#endif
#ifndef LED_PILE
#define LED_PILE 2048
#endif
#ifndef BLE_NOTIFY_PERIOD_MS
#define BLE_NOTIFY_PERIOD_MS 250
#endif
#ifndef GESTE_LONG_MS
#define GESTE_LONG_MS 600
#endif
#ifndef GESTE_TRES_LONG_MS
#define GESTE_TRES_LONG_MS 3000
#endif
#ifndef GESTE_ATTENTE_MS
#define GESTE_ATTENTE_MS 400
#endif
#ifndef PAS_VOLUME_BOUTON
#define PAS_VOLUME_BOUTON 10
#endif
#ifndef WIFI_OTA_DUREE_S
#define WIFI_OTA_DUREE_S 300
#endif
#ifndef WIFI_OTA_PRIO
#define WIFI_OTA_PRIO 2
#endif
#ifndef WIFI_OTA_PILE
#define WIFI_OTA_PILE 6144
#endif
