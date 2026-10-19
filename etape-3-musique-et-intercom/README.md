# Étape 3 : la musique et l'intercom ensemble

On réunit les étapes 1 et 2 : chaque casque reçoit sa musique du téléphone en Bluetooth, et la voix passe toujours entre les deux intercoms en ESP-NOW. Quand l'autre parle, la musique baisse automatiquement puis remonte. C'est aussi l'étape où l'intercom devient autonome sur batterie.

Le code compile avec PlatformIO mais n'a pas encore été testé sur de vraies cartes. C'est l'étape la plus risquée : voir « Points à vérifier ».

## Comment ça marche

- La musique arrive à 44,1 kHz (parfois 48 kHz), la voix à 16 kHz. La voix est rééchantillonnée à la volée.
- Une seule tâche mélange les deux et alimente l'ampli, ce qui évite que l'un coupe l'autre.
- La musique descend à 20 % du volume pendant la voix (réglable avec `MUSIQUE_PENDANT_VOIX`).
- Le Bluetooth et le Wi-Fi partagent l'antenne de l'ESP32 et se relaient : le tampon de la voix passe à 90 ms pour absorber ces pauses.
- Le micro fonctionne uniquement en déclenchement à la voix (VOX).

## Câblage

Identique à l'étape 2. Pour la version sur batterie :

| De | Vers |
| --- | --- |
| Batterie 18650 | B+ et B− du TP4056 |
| OUT+ du TP4056 | Interrupteur, puis IN+ du MT3608 |
| OUT− du TP4056 | IN− du MT3608 |
| OUT+ du MT3608 (réglé à **5,0 V au multimètre avant de brancher**) | **5V** de l'ESP32 et VIN du MAX98357A |
| OUT− du MT3608 | GND |

On recharge par le port USB-C du TP4056. Coupe l'interrupteur avant de brancher l'USB de l'ESP32 pour le programmer.

Autonomie estimée : 6 à 8 heures avec une 18650 de 3 000 mAh. C'est à mesurer.

## Les boutons

| Bouton | Appui court | Appui long |
| --- | --- | --- |
| A | Lecture / pause | Couper / rétablir le micro |
| B | Volume + | Piste suivante |
| C | Volume − | Piste précédente |

LED bleue : fixe quand le téléphone est connecté, clignotante quand l'autre parle.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h` sur chaque intercom. Même groupe et même canal, mais **un nom Bluetooth différent** pour chacun (« Intercom Pilote », « Intercom Passager »).
2. Téléverse sur les deux cartes, ouvre `pio device monitor`.
3. Connecte un téléphone à chaque intercom, lance la musique, puis parle dans un micro : la musique de l'autre casque doit baisser et ta voix passer par-dessus.

## Points à vérifier ensemble

- **Voix hachée quand la musique joue** : c'est le partage d'antenne Bluetooth / Wi-Fi. Compare les paquets perdus musique en pause et musique en lecture. Si c'est trop, on peut agrandir le tampon de la voix (`PREREMP_VOIX`), au prix d'un peu plus de retard.
- **Mémoire** : le moniteur affiche la mémoire libre au démarrage. Au-dessous de 30 000 octets, il faudra alléger.
- **Craquements dans la musique** : le moniteur affiche la musique en réserve. Si elle tombe souvent à 0, le Bluetooth n'arrive pas à suivre ; essaie avec le téléphone plus près du boîtier.
- **Réglage VOX en roulant** : comme à l'étape 2, à refaire avec le vent.
