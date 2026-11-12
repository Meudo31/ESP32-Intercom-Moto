#include "led_task.h"

#include <Arduino.h>
#include "parametres.h"
#include "system_events.h"

static void tacheLed(void *) {
  pinMode(BROCHE_LED, OUTPUT);
  uint32_t n = 0;
  for (;;) {
    EventBits_t e = sysGet();
    bool allume;
    if (e & SYS_OTA) allume = (n % 2) == 0;                    // 5 Hz
    else if (e & SYS_TEL_CONNECTE) allume = true;
    else if (e & SYS_BLE_CONNECTED) allume = (n % 20) == 0;    // éclair toutes les 2 s
    else allume = false;
    digitalWrite(BROCHE_LED, allume);
    n++;
    vTaskDelay(pdMS_TO_TICKS(100));
  }
}

void ledDemarrer() {
  xTaskCreate(tacheLed, "led", LED_PILE, nullptr, LED_PRIO, nullptr);
}
