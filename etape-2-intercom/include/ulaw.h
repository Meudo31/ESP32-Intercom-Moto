// Compression G.711 µ-law : 16 bits -> 8 bits par échantillon.
// C'est le codec des téléphones fixes : simple, sans latence, largement suffisant pour la voix.
// Il divise par deux le débit radio (128 kbit/s au lieu de 256 à 16 kHz).
#pragma once
#include <stdint.h>

static inline uint8_t ulawEncoder(int16_t echantillon) {
  const int16_t BIAIS = 0x84;
  const int16_t MAXI = 32635;
  int signe = (echantillon >> 8) & 0x80;
  int v = echantillon;
  if (signe) v = -v;
  if (v > MAXI) v = MAXI;
  v += BIAIS;
  int exposant = 7;
  for (int masque = 0x4000; (v & masque) == 0 && exposant > 0; masque >>= 1) exposant--;
  int mantisse = (v >> (exposant + 3)) & 0x0F;
  return ~(signe | (exposant << 4) | mantisse);
}

static inline int16_t ulawDecoder(uint8_t octet) {
  octet = ~octet;
  int signe = octet & 0x80;
  int exposant = (octet >> 4) & 0x07;
  int mantisse = octet & 0x0F;
  int v = ((mantisse << 3) + 0x84) << exposant;
  v -= 0x84;
  return signe ? -v : v;
}
