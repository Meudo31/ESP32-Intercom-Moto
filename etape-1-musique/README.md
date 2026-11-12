# Étape 1 : la musique Bluetooth dans le casque

L'ESP32 se présente au téléphone comme une enceinte Bluetooth nommée « Intercom Moto ». La musique est envoyée en numérique (I2S) à l'ampli MAX98357A, qui alimente les deux haut-parleurs du casque. Par défaut, il n'y a aucun bouton : le volume, la lecture et les pistes se règlent depuis le téléphone. Trois boutons peuvent être ajoutés en option.

Carte conseillée : **D1 mini ESP32** (module WROOM-32, environ 39 × 31 mm). Toute carte ESP32 classique convient ; pas d'ESP32-S3, C3, C6 ni P4, qui n'ont pas de Bluetooth classique.

Le code compile avec PlatformIO (ESP32-WROOM-32, framework Arduino, bibliothèque ESP32-A2DP 1.8.7) mais n'a pas encore été testé sur une vraie carte.

## Câblage

| De | Vers |
| --- | --- |
| MAX98357A **VIN** | **5V** (ou VIN) de l'ESP32 |
| MAX98357A **GND** | GND |
| MAX98357A **BCLK** | GPIO26 |
| MAX98357A **LRC** | GPIO25 |
| MAX98357A **DIN** | GPIO22 |
| MAX98357A **GAIN** et **SD** | non branchés (gain 9 dB, son mono gauche + droite) |
| MAX98357A sorties **+** et **−** | les deux haut-parleurs **en parallèle** (+ avec +, − avec −) |

Sur la D1 mini ESP32, les broches sont marquées IO26, IO25 et IO22 ; la LED bleue de la carte est sur GPIO2.

Deux haut-parleurs 8 Ω en parallèle font 4 Ω, ce que le MAX98357A accepte. Pendant les essais, la carte est alimentée par son port USB.

## Les boutons (option)

Sans boutons, c'est le téléphone qui commande tout. Pour en ajouter, mets `AVEC_BOUTONS` à 1 dans `config.h` et câble :

| Bouton | Broche |
| --- | --- |
| A | GPIO32 et GND |
| B | GPIO33 et GND |
| C | GPIO27 et GND |

| Bouton | Appui court | Appui long |
| --- | --- | --- |
| A | Lecture / pause | |
| B | Volume + | Piste suivante |
| C | Volume − | Piste précédente |

Le volume du téléphone s'applique en plus du volume de l'intercom.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h` (tu peux changer le nom Bluetooth et activer les boutons).
2. Branche la carte en USB, lance `pio run -t upload`, puis `pio device monitor`.
3. Sur le téléphone, cherche « Intercom Moto » dans le Bluetooth et appaire-le. La LED bleue de la carte s'allume une fois connecté.
4. Lance une musique : elle sort dans les haut-parleurs. Règle le volume depuis le téléphone (ou avec les boutons si tu les as ajoutés).

La carte se reconnecte toute seule au dernier téléphone appairé au démarrage suivant.

## Points à vérifier ensemble

- **Son trop faible** : relie la broche GAIN du MAX98357A à GND (12 dB) ou à GND par une résistance de 100 kΩ (15 dB).
- **Grésillement** : souvent l'alimentation USB du PC. Essaie un chargeur de téléphone, et garde les fils I2S courts.
- **Fréquence** : le moniteur série affiche la fréquence choisie par le téléphone (44 100 ou 48 000 Hz). Les deux fonctionnent.
