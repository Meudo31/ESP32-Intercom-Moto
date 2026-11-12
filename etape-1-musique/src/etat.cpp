#include "etat.h"
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

EtatMusique g_etat;
static SemaphoreHandle_t s_mutex = nullptr;

void etatInit() { if (!s_mutex) s_mutex = xSemaphoreCreateMutex(); }
void etatLock() { xSemaphoreTake(s_mutex, portMAX_DELAY); }
void etatUnlock() { xSemaphoreGive(s_mutex); }
