// Reconnaissance des combinaisons d'appuis sur un seul bouton (entre la broche et GND).
//   court            -> COURT          court + court  -> DOUBLE
//   court + long     -> COURT_LONG     long           -> LONG
//   long + court     -> LONG_COURT     long + long    -> LONG_LONG
//   très long (maintenu)  -> TRES_LONG, dès que la durée est atteinte
// Un geste d'un seul appui est décidé quand aucun second appui ne suit dans GESTE_ATTENTE_MS.
#pragma once
#include <Arduino.h>

enum class Geste { AUCUN, COURT, DOUBLE, COURT_LONG, LONG, LONG_COURT, LONG_LONG, TRES_LONG };

class BoutonGestes {
public:
  BoutonGestes(uint8_t broche, uint32_t longMs, uint32_t tresLongMs, uint32_t attenteMs)
      : broche(broche), longMs(longMs), tresLongMs(tresLongMs), attenteMs(attenteMs) {}

  void begin() { pinMode(broche, INPUT_PULLUP); }

  // À appeler souvent (toutes les 5 ms environ). Renvoie le geste reconnu, ou AUCUN.
  Geste lire() {
    uint32_t t = millis();
    bool brut = digitalRead(broche) == LOW;
    if (brut != etatBrut) {            // changement : on relance l'anti-rebond
      etatBrut = brut;
      dernierChangement = t;
    }
    if (t - dernierChangement < 30) return Geste::AUCUN;
    bool enfonce = etatBrut;

    if (ignorer) {                     // après un appui très long : attendre le relâchement
      if (!enfonce) { ignorer = false; n = 0; }
      return Geste::AUCUN;
    }
    if (enfonce && !etatStable) {      // début d'appui
      etatStable = true;
      debut = t;
    } else if (enfonce && etatStable && t - debut >= tresLongMs) {
      etatStable = false;
      ignorer = true;
      n = 0;
      return Geste::TRES_LONG;
    } else if (!enfonce && etatStable) {   // fin d'appui
      etatStable = false;
      appuiLong[n++] = (t - debut) >= longMs;
      finAppui = t;
      if (n == 2) {
        n = 0;
        if (!appuiLong[0]) return appuiLong[1] ? Geste::COURT_LONG : Geste::DOUBLE;
        return appuiLong[1] ? Geste::LONG_LONG : Geste::LONG_COURT;
      }
    }
    if (!enfonce && n == 1 && t - finAppui >= attenteMs) {   // pas de second appui
      n = 0;
      return appuiLong[0] ? Geste::LONG : Geste::COURT;
    }
    return Geste::AUCUN;
  }

private:
  uint8_t broche;
  uint32_t longMs, tresLongMs, attenteMs;
  bool etatBrut = false, etatStable = false, ignorer = false;
  bool appuiLong[2] = {false, false};
  uint8_t n = 0;
  uint32_t dernierChangement = 0, debut = 0, finAppui = 0;
};
