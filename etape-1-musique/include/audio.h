// Sortie audio : I2S vers l'ampli MAX98357A, avec le volume de l'intercom.
#pragma once
#include <stdint.h>

void audioInit();
void audioArreter();
// Broche SD du MAX98357A : allume l'ampli, ou le coupe et le garde coupé pendant la veille profonde.
void ampliAllumer();
void ampliCouperPourVeille();
// Appelée par la pile Bluetooth à chaque paquet de musique (16 bits stéréo).
void audioRecevoir(const uint8_t *donnees, uint32_t taille);
void audioFrequence(uint16_t frequence);
void audioSetVolume(int volume);   // 0 à 100 %
uint8_t audioVolume();
uint32_t audioOctetsRecus();
