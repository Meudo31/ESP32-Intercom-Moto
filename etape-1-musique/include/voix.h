// Voix d'un appel (HFP) : le son du téléphone part vers l'ampli, celui du micro INMP441
// part vers le téléphone. Le son d'appel est en 16 bits mono, 8 000 Hz (ou 16 000 Hz en mSBC).
#pragma once
#include <stdint.h>

void voixInit();                        // crée les tampons et les deux tâches
void voixDemarrer(uint32_t frequence);  // liaison audio d'appel ouverte
void voixArreter();                     // liaison audio fermée
bool voixActive();
uint32_t voixFrequence();
void voixSetVolume(uint8_t v);          // volume de l'appel, 0 à 15 (échelle HFP)

// Fonctions données à la pile Bluetooth (esp_hf_client_register_data_callback)
void voixRecu(const uint8_t *donnees, uint32_t taille);
uint32_t voixAEnvoyer(uint8_t *donnees, uint32_t taille);
