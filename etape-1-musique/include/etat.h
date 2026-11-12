// Informations sur le téléphone et le morceau en cours, partagées entre tâches (mutex).
#pragma once
#include <stdint.h>

struct EtatMusique {
  bool avrcp = false;          // télécommande (lecture, pistes, volume) disponible
  uint16_t frequence = 44100;  // Hz
  uint32_t positionMs = 0;
  int8_t rssi = 0;             // écart de signal avec le téléphone (dB)
  // Partie « MEDIA » : chaque changement incrémente compteur
  uint32_t compteur = 0;
  char telNom[64] = "";
  char telAdresse[18] = "";
  char titre[96] = "";
  char artiste[64] = "";
  char album[64] = "";
  char genre[32] = "";
  uint16_t piste = 0, nbPistes = 0;
  uint32_t dureeMs = 0;
};

void etatInit();
void etatLock();
void etatUnlock();
extern EtatMusique g_etat;   // à lire ou modifier entre etatLock() et etatUnlock()

// Verrou automatique : { EtatVerrou v; ... }
struct EtatVerrou {
  EtatVerrou() { etatLock(); }
  ~EtatVerrou() { etatUnlock(); }
};
