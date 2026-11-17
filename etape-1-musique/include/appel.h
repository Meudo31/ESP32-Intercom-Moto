// Appels téléphoniques : profil Bluetooth « mains libres » (HFP, rôle kit mains libres).
// Le téléphone voit l'intercom comme un casque d'appel en plus de l'enceinte musique.
#pragma once
#include <stddef.h>
#include <stdint.h>

enum class EtatAppel : uint8_t {
  AUCUN,      // pas d'appel
  SONNERIE,   // appel entrant, pas encore décroché
  SORTANT,    // appel lancé depuis le téléphone, ça sonne chez l'autre
  EN_COURS,   // appel décroché
};

void appelDemarrer();   // après musiqueDemarrer() (la pile Bluetooth doit tourner)
EtatAppel appelEtat();
const char *appelEtatTexte();
void appelNumero(char *dst, size_t taille);   // numéro de l'appelant, si le téléphone le donne

void appelDecrocher();
void appelRejeter();
void appelRaccrocher();
void appelVolumePlus(int pas);   // volume de l'appel (0 à 15), aussi transmis au téléphone
uint8_t appelVolume();
// « Déconnecter » / « Oublier » dans l'app : coupe aussi les appels et arrête la reconnexion auto.
void appelDeconnecter();
void appelReconnexionAuto();   // « Reconnecter » : la reconnexion automatique reprend
