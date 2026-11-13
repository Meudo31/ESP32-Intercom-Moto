// États globaux partagés entre tâches (EventGroup FreeRTOS).
#pragma once
#include <freertos/FreeRTOS.h>
#include <freertos/event_groups.h>

#define SYS_TEL_CONNECTE  (1 << 0)   // téléphone connecté pour la musique (A2DP)
#define SYS_LECTURE       (1 << 1)   // musique en cours
#define SYS_BLE_CONNECTED (1 << 2)   // app Android connectée
#define SYS_OTA           (1 << 3)   // mise à jour du firmware en cours

void sysInit();
void sysSet(EventBits_t bits);
void sysClear(EventBits_t bits);
bool sysIs(EventBits_t bits);
EventBits_t sysGet();
