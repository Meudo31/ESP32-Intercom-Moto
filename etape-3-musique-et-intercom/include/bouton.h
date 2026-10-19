// Bouton poussoir entre une broche et GND, avec anti-rebond, appui court et appui long.
// Pensé pour être utilisé avec des gants : l'appui long se déclenche dès que la durée
// est atteinte, sans attendre le relâchement.
#pragma once
#include <Arduino.h>

enum class Appui { AUCUN, COURT, LONG };

class Bouton {
public:
  explicit Bouton(uint8_t broche, uint32_t dureeLongMs = 700)
      : broche(broche), dureeLong(dureeLongMs) {}

  void begin() { pinMode(broche, INPUT_PULLUP); }

  // À appeler souvent dans loop(). Renvoie l'événement détecté, ou AUCUN.
  Appui lire() {
    bool enfonce = digitalRead(broche) == LOW;
    uint32_t maintenant = millis();
    if (enfonce != etatBrut) {        // changement : on relance l'anti-rebond
      etatBrut = enfonce;
      dernierChangement = maintenant;
    }
    if (maintenant - dernierChangement < 30) return Appui::AUCUN;

    if (enfonce && !etatStable) {     // début d'appui confirmé
      etatStable = true;
      debutAppui = maintenant;
      longSignale = false;
    } else if (enfonce && etatStable && !longSignale && maintenant - debutAppui >= dureeLong) {
      longSignale = true;
      return Appui::LONG;
    } else if (!enfonce && etatStable) {  // relâchement confirmé
      etatStable = false;
      if (!longSignale) return Appui::COURT;
    }
    return Appui::AUCUN;
  }

  // Vrai tant que le bouton est maintenu (utile pour « appuyer pour parler »).
  bool estEnfonce() const { return etatStable; }

private:
  uint8_t broche;
  uint32_t dureeLong;
  bool etatBrut = false, etatStable = false, longSignale = false;
  uint32_t dernierChangement = 0, debutAppui = 0;
};
