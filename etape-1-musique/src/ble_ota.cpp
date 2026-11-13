#include "ble_ota.h"

#include <Arduino.h>
#include <Update.h>
#include <stdlib.h>
#include "parametres.h"
#include "system_events.h"
#include "wifi_ota.h"

static volatile bool s_active = false;
static volatile bool s_restart = false;
static size_t s_expected = 0, s_received = 0;
static uint32_t s_lastLog = 0;

bool bleOtaInProgress() { return s_active; }
bool bleOtaRestartPending() { return s_restart; }

static std::string fail(const char *why) {
  Serial.printf("[OTA] Erreur : %s\n", why);
  if (Update.isRunning()) Update.abort();
  s_active = false;
  sysClear(SYS_OTA);
  return std::string("ERR ") + why;
}

void bleOtaAbort() {
  if (s_active) fail("transfert interrompu");
}

std::string bleOtaControl(const std::string &cmd) {
  if (cmd.rfind("BEGIN ", 0) == 0) {
    // BEGIN <taille> <md5> <clé>
    char md5[40] = {0}, key[64] = {0};
    unsigned long size = 0;
    if (sscanf(cmd.c_str() + 6, "%lu %39s %63s", &size, md5, key) < 3) return "ERR commande BEGIN invalide";
    if (strcmp(key, OTA_KEY) != 0) return "ERR cle OTA invalide";
    if (wifiOtaActif()) return "ERR mise a jour Wi-Fi en cours";
    if (s_active) Update.abort();
    if (size == 0 || size > ESP.getFreeSketchSpace())
      return "ERR fichier trop gros pour la partition OTA";

    s_active = true;
    sysSet(SYS_OTA);
    s_expected = size;
    s_received = 0;
    if (!Update.begin(size, U_FLASH)) return fail(Update.errorString());
    if (strlen(md5) == 32) Update.setMD5(md5);
    Serial.printf("[OTA] Reception de %lu octets par Bluetooth\n", size);
    return "OK";
  }

  if (cmd == "END") {
    if (!s_active) return "ERR aucune mise a jour en cours";
    if (s_received != s_expected) return fail("taille recue incorrecte");
    if (!Update.end(true)) return fail(Update.errorString());   // vérifie aussi le MD5
    Serial.println("[OTA] Mise a jour reussie, redemarrage...");
    s_active = false;
    s_restart = true;
    return "DONE";
  }

  if (cmd.rfind("WIFI ", 0) == 0) {
    // WIFI <clé> : ouvre le point d'accès Wi-Fi pour une mise à jour plus rapide
    if (cmd.substr(5) != OTA_KEY) return "ERR cle OTA invalide";
    return wifiOtaDemarrer();
  }

  if (cmd == "ABORT") {
    if (s_active) fail("annulee par l'app");
    return "OK";
  }
  return "ERR commande inconnue";
}

std::string bleOtaData(const uint8_t *data, size_t len) {
  if (!s_active) return "ERR aucune mise a jour en cours";
  if (s_received + len > s_expected) return fail("plus de donnees que prevu");
  if (s_received == 0 && len > 0 && data[0] != 0xE9) return fail("ce n'est pas un firmware ESP32");
  if (Update.write((uint8_t *)data, len) != len) return fail(Update.errorString());
  s_received += len;
  if (millis() - s_lastLog > 2000) {
    s_lastLog = millis();
    Serial.printf("[OTA] %u / %u octets\n", (unsigned)s_received, (unsigned)s_expected);
  }
  return "";
}
