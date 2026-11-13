// Sortie audio : I2S vers l'ampli MAX98357A, avec le volume de l'intercom.
// Trois sources se partagent l'ampli, une seule à la fois (le « mode ») :
// la musique (A2DP), la sonnerie d'appel et la voix d'un appel (HFP).
#pragma once
#include <stddef.h>
#include <stdint.h>

enum class ModeAudio : uint8_t { MUSIQUE, SONNERIE, APPEL };

void audioInit();
void audioArreter();
// Broche SD du MAX98357A : allume l'ampli, ou le coupe et le garde coupé pendant la veille profonde.
void ampliAllumer();
void ampliCouperPourVeille();
// Gain du MAX98357A : 6, 9 ou 12 dB (autre valeur ramenée à la plus proche). Renvoie le gain appliqué.
uint8_t ampliSetGain(uint8_t db);
uint8_t ampliGain();
// Appelée par la pile Bluetooth à chaque paquet de musique (16 bits stéréo).
void audioRecevoir(const uint8_t *donnees, uint32_t taille);
void audioFrequence(uint16_t frequence);
void audioSetVolume(int volume);   // 0 à 100 %
uint8_t audioVolume();
uint32_t audioOctetsRecus();
// Change la source de l'ampli. frequence : celle de la sonnerie ou de la voix (ignorée pour la musique).
void audioSetMode(ModeAudio mode, uint32_t frequence = 0);
ModeAudio audioMode();
// Envoie des échantillons mono 16 bits sur les deux voies, avec le volume de l'intercom
// multiplié par gainQ15 (32768 = 1,0). Ne fait rien si l'ampli n'est pas en mode « mode ».
void audioEcrireMono(const int16_t *ech, size_t n, int32_t gainQ15, ModeAudio mode);
