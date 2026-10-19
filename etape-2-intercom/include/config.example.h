// Copier ce fichier en config.h et adapter les valeurs.
// Les deux intercoms doivent avoir le MÊME groupe et le MÊME canal.
#pragma once

// Numéro de groupe de 1 à 255 : les intercoms ne s'écoutent qu'entre eux.
// Change-le si tu croises un autre motard qui a fabriqué le même intercom.
#define GROUPE_INTERCOM 42

// Canal Wi-Fi utilisé par ESP-NOW (1, 6 ou 11 conseillés)
#define CANAL_WIFI 1

// 0 = déclenchement à la voix (VOX) : le micro s'ouvre tout seul quand tu parles.
// 1 = appuyer pour parler : le micro n'est ouvert que bouton A enfoncé.
#define MODE_APPUYER_POUR_PARLER 0

// Seuil du VOX (niveau moyen du micro). Plus haut = moins sensible au vent.
// Regarde les niveaux affichés sur le moniteur série pour le régler.
#define SEUIL_VOX 500

// Amplification du micro (1 à 16)
#define GAIN_MICRO 4

// L'INMP441 a sa broche L/R reliée à GND, donc il parle sur le canal gauche.
// Si le moniteur série affiche un niveau toujours à 0, mets 1 ici.
#define MICRO_CANAL_DROIT 0

// Volume de départ des haut-parleurs, de 0 à 100 %
#define VOLUME_DEPART 70

// 1 = mode longue portée Espressif (débit réduit, portée meilleure). À tester sur les deux
// intercoms en même temps : ils doivent avoir la même valeur.
#define LONGUE_PORTEE 0
