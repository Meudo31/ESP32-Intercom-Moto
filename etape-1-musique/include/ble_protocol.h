// Contrat Bluetooth basse consommation (BLE) entre l'intercom et l'app Android.
// À garder identique à BleProtocol.java.
#pragma once
#include <stdint.h>

#define BLE_UUID_SERVICE  "8a5c0001-2763-4123-8f80-5a3b7b30e82c"
#define BLE_UUID_DATA     "8a5c0002-2763-4123-8f80-5a3b7b30e82c"  // read, notify : paquet ci-dessous
#define BLE_UUID_MEDIA    "8a5c0003-2763-4123-8f80-5a3b7b30e82c"  // read : JSON téléphone + morceau
#define BLE_UUID_INFO     "8a5c0004-2763-4123-8f80-5a3b7b30e82c"  // read : JSON carte et firmware
#define BLE_UUID_COMMAND  "8a5c0005-2763-4123-8f80-5a3b7b30e82c"  // write : "cle=valeur&cle2=valeur2"
#define BLE_UUID_OTA_CTRL "8a5c0006-2763-4123-8f80-5a3b7b30e82c"  // write, notify : BEGIN / END / ABORT
#define BLE_UUID_OTA_DATA "8a5c0007-2763-4123-8f80-5a3b7b30e82c"  // write : morceaux du .bin

// Paquet DATA, version 1, little-endian, 32 octets :
//  0  u8   version (1)
//  1  u8   drapeaux : bit0 téléphone connecté (musique), bit1 lecture en cours,
//                     bit2 télécommande AVRCP connectée, bit3 boutons B et C présents,
//                     bit4 mise à jour en cours
//  2  u8   volume de l'intercom, 0 à 100 %
//  3  u8   volume envoyé par le téléphone, 0 à 127
//  4  u16  fréquence audio (Hz)
//  6  u32  temps depuis le démarrage (s)
// 10  u32  mémoire libre (octets)
// 14  u32  mémoire libre minimale depuis le démarrage (octets)
// 18  u32  octets audio reçus depuis le démarrage
// 22  u32  position dans le morceau (ms)
// 26  i8   écart de signal Bluetooth avec le téléphone (dB, 0 = bon)
// 27  u8   veille automatique (minutes, 0 = jamais)
// 28  u32  compteur de changements de MEDIA : relire MEDIA quand il change
#define DATA_VERSION 1
#define DATA_TAILLE 32

#define DRAPEAU_TEL       0x01
#define DRAPEAU_LECTURE   0x02
#define DRAPEAU_AVRCP     0x04
#define DRAPEAU_BOUTONS   0x08
#define DRAPEAU_OTA       0x10

// Commandes (COMMAND), séparées par & :
//   cmd=play | pause | basculer | suivant | precedent | stop
//   vol=0..100        volume de l'intercom
//   voltel=0..127     volume du téléphone (volume absolu AVRCP)
//   veille=1          veille profonde (réveil : bouton A)
//   veille_auto=0..240  minutes sans connexion avant la veille, 0 = jamais
//   redemarrer=1
//   nom=<texte>       nouveau nom Bluetooth (sans & ni =, 1 à 29 caractères), puis redémarrage
//   deconnecter=1     déconnecte le téléphone
//   reconnecter=1     se reconnecte au dernier téléphone
//   oublier=1         oublie le dernier téléphone (il faudra le réappairer)
