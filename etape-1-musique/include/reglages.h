// Réglages mémorisés dans la flash (NVS) : ils survivent au redémarrage et à la veille.
#pragma once
#include <stdint.h>

void reglagesCharger();
const char *reglagesNom();
void reglagesSetNom(const char *nom);
uint8_t reglagesVolume();
void reglagesSetVolume(uint8_t v);
uint8_t reglagesVeilleAuto();
void reglagesSetVeilleAuto(uint8_t minutes);
