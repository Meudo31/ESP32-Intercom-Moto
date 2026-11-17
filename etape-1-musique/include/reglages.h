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
uint8_t reglagesGain();
void reglagesSetGain(uint8_t db);
uint8_t reglagesGainMicro();
void reglagesSetGainMicro(uint8_t g);
