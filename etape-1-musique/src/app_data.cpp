#include "app_data.h"

#include <Arduino.h>
#include <esp_system.h>
#if __has_include(<esp_mac.h>)
#include <esp_mac.h>
#endif
#include "appel.h"
#include "audio.h"
#include "ble_ota.h"
#include "ble_protocol.h"
#include "commandes.h"
#include "etat.h"
#include "musique_bt.h"
#include "parametres.h"
#include "reglages.h"
#include "system_events.h"

// ---------- paquet DATA ----------

static void put16(uint8_t *p, uint16_t v) { p[0] = v; p[1] = v >> 8; }
static void put32(uint8_t *p, uint32_t v) { for (int i = 0; i < 4; i++) p[i] = v >> (8 * i); }

size_t appDataBuild(uint8_t *out, size_t max) {
  if (max < DATA_TAILLE) return 0;
  EventBits_t e = sysGet();
  EtatMusique m;
  {
    EtatVerrou v;
    m.avrcp = g_etat.avrcp;
    m.frequence = g_etat.frequence;
    m.positionMs = g_etat.positionMs;
    m.rssi = g_etat.rssi;
    m.compteur = g_etat.compteur;
  }
  uint8_t d = 0;
  if (e & SYS_TEL_CONNECTE) d |= DRAPEAU_TEL;
  if (e & SYS_LECTURE) d |= DRAPEAU_LECTURE;
  if (m.avrcp) d |= DRAPEAU_AVRCP;
  if (AVEC_BOUTONS) d |= DRAPEAU_BOUTONS;
  if (e & SYS_OTA) d |= DRAPEAU_OTA;
  if (e & SYS_SONNERIE) d |= DRAPEAU_SONNERIE;
  if (e & SYS_APPEL) d |= DRAPEAU_APPEL;
  if (e & SYS_HFP_CONNECTE) d |= DRAPEAU_HFP;

  out[0] = DATA_VERSION;
  out[1] = d;
  out[2] = audioVolume();
  out[3] = musiqueVolumeTel();
  put16(out + 4, m.frequence);
  put32(out + 6, millis() / 1000);
  put32(out + 10, ESP.getFreeHeap());
  put32(out + 14, ESP.getMinFreeHeap());
  put32(out + 18, audioOctetsRecus());
  put32(out + 22, m.positionMs);
  out[26] = (uint8_t)m.rssi;
  out[27] = reglagesVeilleAuto();
  put32(out + 28, m.compteur);
  out[32] = ampliGain();
  return DATA_TAILLE;
}

// ---------- JSON ----------

static void ajouterTexte(std::string &j, const char *cle, const char *val, bool virgule = true) {
  if (virgule) j += ',';
  j += '"';
  j += cle;
  j += "\":\"";
  for (const char *p = val; *p; p++) {
    char c = *p;
    if (c == '"' || c == '\\') { j += '\\'; j += c; }
    else if ((uint8_t)c < 0x20) j += ' ';
    else j += c;
  }
  j += '"';
}

static void ajouterNombre(std::string &j, const char *cle, long long val) {
  char buf[48];
  snprintf(buf, sizeof buf, ",\"%s\":%lld", cle, val);
  j += buf;
}

std::string appMediaJson() {
  EtatVerrou v;
  std::string j = "{";
  ajouterTexte(j, "tel_nom", g_etat.telNom, false);
  ajouterTexte(j, "tel_adresse", g_etat.telAdresse);
  ajouterTexte(j, "titre", g_etat.titre);
  ajouterTexte(j, "artiste", g_etat.artiste);
  ajouterTexte(j, "album", g_etat.album);
  ajouterTexte(j, "genre", g_etat.genre);
  ajouterNombre(j, "piste", g_etat.piste);
  ajouterNombre(j, "nb_pistes", g_etat.nbPistes);
  ajouterNombre(j, "duree_ms", g_etat.dureeMs);
  char numero[24];
  appelNumero(numero, sizeof numero);
  ajouterTexte(j, "appel", appelEtatTexte());
  ajouterTexte(j, "appel_numero", numero);
  j += '}';
  return j;
}

static const char *raisonDemarrage() {
  switch (esp_reset_reason()) {
    case ESP_RST_POWERON:   return "mise sous tension";
    case ESP_RST_SW:        return "redemarrage logiciel";
    case ESP_RST_PANIC:     return "plantage";
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:       return "chien de garde";
    case ESP_RST_DEEPSLEEP: return "reveil de veille";
    case ESP_RST_BROWNOUT:  return "baisse de tension";
    case ESP_RST_EXT:       return "bouton reset";
    default:                return "inconnue";
  }
}

std::string appInfoJson() {
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_BT);
  char macTxt[18];
  snprintf(macTxt, sizeof macTxt, "%02X:%02X:%02X:%02X:%02X:%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);

  std::string j = "{";
  ajouterTexte(j, "name", reglagesNom(), false);
  ajouterTexte(j, "fw", FW_VERSION);
  ajouterTexte(j, "chip", ESP.getChipModel());
  ajouterNombre(j, "chip_rev", ESP.getChipRevision());
  ajouterNombre(j, "cores", ESP.getChipCores());
  ajouterNombre(j, "cpu_mhz", ESP.getCpuFreqMHz());
  ajouterNombre(j, "flash", ESP.getFlashChipSize());
  ajouterNombre(j, "sketch_size", ESP.getSketchSize());
  ajouterNombre(j, "free_sketch", ESP.getFreeSketchSpace());
  ajouterNombre(j, "heap_total", ESP.getHeapSize());
  ajouterTexte(j, "mac", macTxt);
  ajouterTexte(j, "sdk", ESP.getSdkVersion());
  ajouterTexte(j, "reset", raisonDemarrage());
  ajouterNombre(j, "boutons", AVEC_BOUTONS);
  ajouterNombre(j, "micro", AVEC_MICRO);
  ajouterTexte(j, "build", __DATE__ " " __TIME__);
  j += '}';
  return j;
}

// ---------- commandes ----------

static void traiter(const char *cle, const char *val) {
  Commande c;
  long n = atol(val);
  if (!strcmp(cle, "cmd")) {
    if (!strcmp(val, "play")) c.type = Cmd::LECTURE;
    else if (!strcmp(val, "pause")) c.type = Cmd::PAUSE;
    else if (!strcmp(val, "basculer")) c.type = Cmd::BASCULER;
    else if (!strcmp(val, "stop")) c.type = Cmd::STOP;
    else if (!strcmp(val, "suivant")) c.type = Cmd::SUIVANT;
    else if (!strcmp(val, "precedent")) c.type = Cmd::PRECEDENT;
    else { Serial.printf("[App] Commande inconnue : %s\n", val); return; }
  } else if (!strcmp(cle, "vol")) { c.type = Cmd::VOLUME; c.valeur = n; }
  else if (!strcmp(cle, "voltel")) { c.type = Cmd::VOLUME_TEL; c.valeur = n; }
  else if (!strcmp(cle, "veille")) c.type = Cmd::VEILLE;
  else if (!strcmp(cle, "veille_auto")) { c.type = Cmd::VEILLE_AUTO; c.valeur = n; }
  else if (!strcmp(cle, "gain")) { c.type = Cmd::GAIN; c.valeur = n; }
  else if (!strcmp(cle, "redemarrer")) c.type = Cmd::REDEMARRER;
  else if (!strcmp(cle, "nom")) {
    if (!*val) return;
    c.type = Cmd::NOM;
    strlcpy(c.texte, val, 30);
  }
  else if (!strcmp(cle, "deconnecter")) c.type = Cmd::DECONNECTER;
  else if (!strcmp(cle, "reconnecter")) c.type = Cmd::RECONNECTER;
  else if (!strcmp(cle, "oublier")) c.type = Cmd::OUBLIER;
  else if (!strcmp(cle, "appel")) {
    if (!strcmp(val, "decrocher")) c.type = Cmd::DECROCHER;
    else if (!strcmp(val, "rejeter")) c.type = Cmd::REJETER;
    else if (!strcmp(val, "raccrocher")) c.type = Cmd::RACCROCHER;
    else return;
  }
  else if (!strcmp(cle, "volappel")) { c.type = Cmd::VOLUME_APPEL_PLUS; c.valeur = n - appelVolume(); }
  else { Serial.printf("[App] Réglage inconnu : %s\n", cle); return; }
  commandeEnvoyer(c);
}

void appCommand(const char *text, size_t len) {
  std::string s(text, len);
  Serial.printf("[App] %s\n", s.c_str());
  size_t debut = 0;
  while (debut <= s.size()) {
    size_t fin = s.find('&', debut);
    if (fin == std::string::npos) fin = s.size();
    std::string paire = s.substr(debut, fin - debut);
    size_t eg = paire.find('=');
    if (eg != std::string::npos) traiter(paire.substr(0, eg).c_str(), paire.substr(eg + 1).c_str());
    debut = fin + 1;
  }
}
