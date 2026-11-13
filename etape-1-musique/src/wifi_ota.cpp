#include "wifi_ota.h"

#include <Arduino.h>
#include <Update.h>
#include <WebServer.h>
#include <WiFi.h>
#include <esp_random.h>
#include "ble_ota.h"
#include "parametres.h"
#include "system_events.h"

// Réponses HTTP : 200 "DONE" (l'intercom redémarre) ou 4xx/5xx "ERR <raison>".
// Requête : POST http://192.168.4.1/update?key=<OTA_KEY>&md5=<md5>  (corps multipart/form-data, champ "firmware")

static volatile bool s_actif = false;
static char s_ssid[24], s_mdp[16];
static WebServer *s_serveur = nullptr;
static String s_erreur;
static bool s_reussi = false;

bool wifiOtaActif() { return s_actif; }

static void echec(const String &raison) {
  if (s_erreur.isEmpty()) s_erreur = raison;
  if (Update.isRunning()) Update.abort();
  Serial.printf("[OTA Wi-Fi] Erreur : %s\n", raison.c_str());
}

static void recevoirMorceau() {
  HTTPUpload &up = s_serveur->upload();
  if (up.status == UPLOAD_FILE_START) {
    s_erreur = "";
    s_reussi = false;
    if (s_serveur->arg("key") != OTA_KEY) return echec("cle OTA invalide");
    if (bleOtaInProgress()) return echec("mise a jour Bluetooth deja en cours");
    sysSet(SYS_OTA);
    if (!Update.begin(UPDATE_SIZE_UNKNOWN, U_FLASH)) return echec(Update.errorString());
    String md5 = s_serveur->arg("md5");
    if (md5.length() == 32) Update.setMD5(md5.c_str());
    Serial.printf("[OTA Wi-Fi] Réception de %s\n", up.filename.c_str());
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (!s_erreur.isEmpty() || !Update.isRunning()) return;
    if (up.totalSize == 0 && up.currentSize > 0 && up.buf[0] != 0xE9) return echec("ce n'est pas un firmware ESP32");
    if (Update.write(up.buf, up.currentSize) != up.currentSize) echec(Update.errorString());
  } else if (up.status == UPLOAD_FILE_END) {
    if (!s_erreur.isEmpty()) return;
    if (!Update.end(true)) return echec(Update.errorString());   // vérifie aussi le MD5
    s_reussi = true;
    Serial.printf("[OTA Wi-Fi] %u octets reçus, firmware valide\n", (unsigned)up.totalSize);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    echec("transfert interrompu");
  }
}

static void reponseFinale() {
  if (s_reussi) {
    s_serveur->send(200, "text/plain", "DONE");
  } else {
    s_serveur->send(400, "text/plain", "ERR " + (s_erreur.isEmpty() ? String("fichier manquant") : s_erreur));
    sysClear(SYS_OTA);
  }
}

static void tacheWifiOta(void *) {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(s_ssid, s_mdp);
  Serial.printf("[OTA Wi-Fi] Point d'accès « %s » sur %s\n", s_ssid, WiFi.softAPIP().toString().c_str());

  WebServer serveur(80);
  s_serveur = &serveur;
  serveur.on("/update", HTTP_POST, reponseFinale, recevoirMorceau);
  serveur.on("/", HTTP_GET, [&serveur]() { serveur.send(200, "text/plain", "Intercom " FW_VERSION); });
  serveur.begin();

  uint32_t debut = millis();
  while (!s_reussi && millis() - debut < WIFI_OTA_DUREE_S * 1000UL) {
    serveur.handleClient();
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  if (s_reussi) {
    // laisse partir la réponse « DONE » puis redémarre sur le nouveau firmware
    uint32_t t = millis();
    while (millis() - t < 1000) { serveur.handleClient(); vTaskDelay(pdMS_TO_TICKS(5)); }
    Serial.println("[OTA Wi-Fi] Redémarrage");
    ESP.restart();
  }
  Serial.println("[OTA Wi-Fi] Délai dépassé, Wi-Fi coupé");
  serveur.stop();
  s_serveur = nullptr;
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);
  sysClear(SYS_OTA);
  s_actif = false;
  vTaskDelete(nullptr);
}

std::string wifiOtaDemarrer() {
  if (s_actif) return std::string("WIFI ") + s_ssid + " " + s_mdp;
  if (bleOtaInProgress()) return "ERR mise a jour Bluetooth deja en cours";

  // Nom à partir de l'adresse de la carte, mot de passe aléatoire à chaque fois
  uint64_t mac = ESP.getEfuseMac();
  snprintf(s_ssid, sizeof s_ssid, "Intercom-OTA-%04X", (unsigned)((mac >> 32) & 0xFFFF));
  static const char lettres[] = "abcdefghjkmnpqrstuvwxyz23456789";
  for (int i = 0; i < 12; i++) s_mdp[i] = lettres[esp_random() % (sizeof lettres - 1)];
  s_mdp[12] = 0;

  s_actif = true;
  s_reussi = false;
  if (xTaskCreate(tacheWifiOta, "wifi_ota", WIFI_OTA_PILE, nullptr, WIFI_OTA_PRIO, nullptr) != pdPASS) {
    s_actif = false;
    return "ERR memoire insuffisante";
  }
  return std::string("WIFI ") + s_ssid + " " + s_mdp;
}
