// Réception de la musique du téléphone en Bluetooth classique (A2DP) et télécommande (AVRCP).
#pragma once
#include <stdint.h>

void musiqueDemarrer(const char *nom);
void musiqueArreter();

void musiqueLecture();
void musiquePause();
void musiqueBasculer();     // lecture <-> pause
void musiqueStop();
void musiqueSuivant();
void musiquePrecedent();
void musiqueSetVolumeTel(uint8_t v);   // 0 à 127, transmis au téléphone
uint8_t musiqueVolumeTel();
void musiqueDeconnecter();
void musiqueReconnecter();
void musiqueOublier();
bool musiqueAdresseTel(uint8_t adresse[6]);   // adresse du téléphone connecté (musique)
void musiqueRafraichirSignal();   // à appeler régulièrement : mesure du signal
