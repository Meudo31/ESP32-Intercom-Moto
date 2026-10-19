// Tampon circulaire d'échantillons audio 16 bits, partagé entre deux tâches
// (une qui écrit, une qui lit). Protégé par un verrou court.
#pragma once
#include <Arduino.h>

template <size_t TAILLE>
class Anneau {
public:
  // Ajoute n échantillons. Si le tampon est plein, les plus anciens sont écrasés
  // (on préfère perdre du vieux son que prendre du retard).
  void ecrire(const int16_t* src, size_t n) {
    portENTER_CRITICAL(&verrou);
    for (size_t i = 0; i < n; i++) {
      donnees[tete] = src[i];
      tete = (tete + 1) % TAILLE;
      if (compte < TAILLE) compte++;
      else queue = (queue + 1) % TAILLE;
    }
    portEXIT_CRITICAL(&verrou);
  }

  // Lit jusqu'à n échantillons, renvoie le nombre réellement lu.
  size_t lire(int16_t* dst, size_t n) {
    portENTER_CRITICAL(&verrou);
    size_t lus = 0;
    while (lus < n && compte > 0) {
      dst[lus++] = donnees[queue];
      queue = (queue + 1) % TAILLE;
      compte--;
    }
    portEXIT_CRITICAL(&verrou);
    return lus;
  }

  size_t disponible() {
    portENTER_CRITICAL(&verrou);
    size_t n = compte;
    portEXIT_CRITICAL(&verrou);
    return n;
  }

  void vider() {
    portENTER_CRITICAL(&verrou);
    tete = queue = compte = 0;
    portEXIT_CRITICAL(&verrou);
  }

private:
  int16_t donnees[TAILLE];
  size_t tete = 0, queue = 0, compte = 0;
  portMUX_TYPE verrou = portMUX_INITIALIZER_UNLOCKED;
};
