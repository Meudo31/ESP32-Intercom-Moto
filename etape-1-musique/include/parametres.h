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
#define BROCHE_I2S_BCLK 26
#endif
#ifndef BROCHE_I2S_LRC
#define BROCHE_I2S_LRC 25
#endif
#ifndef BROCHE_I2S_DOUT
#define BROCHE_I2S_DOUT 22
#endif
#ifndef BROCHE_LED
#define BROCHE_LED 2
#endif
#ifndef BROCHE_BOUTON_A
#define BROCHE_BOUTON_A 32
#endif
#ifndef BROCHE_BOUTON_B
#define BROCHE_BOUTON_B 33
#endif
#ifndef BROCHE_BOUTON_C
#define BROCHE_BOUTON_C 27
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
