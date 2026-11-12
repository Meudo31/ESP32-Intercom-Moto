#include "reglages.h"
#include <Arduino.h>
#include <Preferences.h>
#include "parametres.h"

static char s_nom[32] = NOM_BLUETOOTH;
static uint8_t s_volume = VOLUME_DEPART;
static uint8_t s_veille = VEILLE_AUTO_MIN;
static Preferences s_prefs;

void reglagesCharger() {
  s_prefs.begin("intercom", false);
  String nom = s_prefs.getString("nom", NOM_BLUETOOTH);
  strlcpy(s_nom, nom.c_str(), sizeof s_nom);
  s_volume = s_prefs.getUChar("volume", VOLUME_DEPART);
  s_veille = s_prefs.getUChar("veille", VEILLE_AUTO_MIN);
  Serial.printf("[Réglages] nom « %s », volume %u %%, veille auto %u min\n", s_nom, s_volume, s_veille);
}

const char *reglagesNom() { return s_nom; }

void reglagesSetNom(const char *nom) {
  strlcpy(s_nom, nom, sizeof s_nom);
  s_prefs.putString("nom", s_nom);
}

uint8_t reglagesVolume() { return s_volume; }

void reglagesSetVolume(uint8_t v) {
  if (v == s_volume) return;
  s_volume = v;
  s_prefs.putUChar("volume", v);
}

uint8_t reglagesVeilleAuto() { return s_veille; }

void reglagesSetVeilleAuto(uint8_t minutes) {
  s_veille = minutes;
  s_prefs.putUChar("veille", minutes);
}
