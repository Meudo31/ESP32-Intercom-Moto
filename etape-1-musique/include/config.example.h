// Copier ce fichier en config.h et adapter les valeurs.
// Pas de mot de passe à cette étape, mais on garde la même habitude que pour les autres projets.
#pragma once

// Nom visible dans la liste Bluetooth du téléphone
#define NOM_BLUETOOTH "Intercom Moto"

// Volume de départ avec boutons, de 0 à 100 %. Le volume du téléphone s'applique en plus.
// Sans boutons, le volume local est à 100 % et seul le téléphone règle le volume.
#define VOLUME_DEPART 60

// Le bouton A (GPIO32, veille profonde et lecture / pause) est toujours présent.
// Boutons B et C : 0 = absents (volume et pistes depuis le téléphone),
// 1 = deux boutons de plus sur GPIO33 et 27 (voir README).
#define AVEC_BOUTONS 0
