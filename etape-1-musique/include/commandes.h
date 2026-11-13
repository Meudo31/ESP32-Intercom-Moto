// File de commandes : boutons et app Android déposent, une seule tâche exécute.
#pragma once
#include <stdint.h>

enum class Cmd : uint8_t {
  LECTURE, PAUSE, BASCULER, STOP, SUIVANT, PRECEDENT,
  VOLUME,         // valeur : 0 à 100
  VOLUME_PLUS,    // valeur : pas
  VOLUME_TEL,     // valeur : 0 à 127
  VOLUME_TEL_PLUS,  // valeur : pas (sur 127)
  VEILLE,
  VEILLE_AUTO,    // valeur : minutes
  GAIN,           // valeur : 6, 9 ou 12 dB
  REDEMARRER,
  NOM,            // texte
  DECONNECTER, RECONNECTER, OUBLIER,
  DECROCHER, REJETER, RACCROCHER,
  VOLUME_APPEL_PLUS,  // valeur : pas (sur 15)
  REGLER_MICRO,       // valeur : gain du micro, 1 à 32
};

struct Commande {
  Cmd type;
  int32_t valeur = 0;
  char texte[32] = "";
};

void commandesDemarrer();
bool commandeEnvoyer(const Commande &c);
bool commandeEnvoyer(Cmd type, int32_t valeur = 0);
