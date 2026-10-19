// Copier ce fichier en config.h et adapter les valeurs.
// Les deux intercoms doivent avoir le MÊME groupe et le MÊME canal.
#pragma once

// Nom visible dans la liste Bluetooth du téléphone. Donne un nom différent à chaque
// intercom (« Intercom Pilote », « Intercom Passager ») pour ne pas les confondre.
#define NOM_BLUETOOTH "Intercom Pilote"

// Numéro de groupe de 1 à 255 : les intercoms ne s'écoutent qu'entre eux.
#define GROUPE_INTERCOM 42

// Canal Wi-Fi utilisé par ESP-NOW (1, 6 ou 11 conseillés)
#define CANAL_WIFI 1

// Seuil du déclenchement à la voix (niveau moyen du micro). Plus haut = moins sensible au vent.
#define SEUIL_VOX 500

// Amplification du micro (1 à 16)
#define GAIN_MICRO 4

// Si le moniteur série affiche un niveau micro toujours à 0, mets 1 ici.
#define MICRO_CANAL_DROIT 0

// Volume général de départ, de 0 à 100 %. Le volume du téléphone s'applique en plus à la musique.
#define VOLUME_DEPART 60

// Volume de la musique pendant que l'autre parle, en % (baisse automatique)
#define MUSIQUE_PENDANT_VOIX 20
