// Sonnerie d'appel entrant : le fichier data/sonnerie.wav, intégré au firmware, joué en boucle
// dans le casque. Formats acceptés : WAV PCM 8 ou 16 bits, mono ou stéréo, 8 000 à 48 000 Hz.
// Le fichier doit rester petit (la place libre dans le firmware est d'environ 250 Ko) :
// 16 000 Hz mono 16 bits = 32 Ko par seconde.
#pragma once
#include <stdint.h>

void sonnerieInit();          // lit l'en-tête du WAV et crée la tâche
void sonnerieDemarrer();      // joue en boucle (l'ampli doit être en mode SONNERIE)
void sonnerieArreter();
bool sonnerieActive();
bool sonnerieDisponible();     // false si le fichier WAV est illisible
uint32_t sonnerieFrequence();  // fréquence d'échantillonnage du fichier (Hz)
