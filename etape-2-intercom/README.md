# Étape 2 : l'intercom entre deux casques

Chaque casque reçoit un micro numérique INMP441. Quand tu parles, ta voix est compressée et envoyée par **ESP-NOW**, la liaison radio directe entre ESP32 : pas de box, pas de téléphone, pas d'appairage. L'autre intercom la joue dans ses haut-parleurs. Les deux peuvent parler en même temps, comme au téléphone.

Il faut **deux** montages identiques. Cette étape n'utilise pas le Bluetooth : on met au point la voix seule avant de tout mélanger à l'étape 3.

Le code compile avec PlatformIO mais n'a pas encore été testé sur de vraies cartes.

## Comment ça marche

- Le micro est lu à 16 kHz (qualité « téléphone HD »).
- Le son est découpé en paquets de 15 ms et compressé en µ-law (le codec des téléphones fixes) : 67 paquets par seconde, 128 kbit/s.
- Les paquets partent en diffusion avec un numéro de groupe : deux intercoms avec le même groupe s'entendent, sans avoir à se connaître.
- À la réception, un petit tampon de 45 ms absorbe les irrégularités de la radio. Si du retard s'accumule, il est jeté pour rester en direct.
- Le micro ne s'ouvre que quand tu parles (VOX), pour ne pas envoyer le bruit du moteur en continu.

## Câblage

Celui de l'étape 1, plus le micro :

| De | Vers |
| --- | --- |
| INMP441 **VDD** | **3V3** de l'ESP32 (surtout pas 5 V) |
| INMP441 **GND** | GND |
| INMP441 **SCK** | GPIO14 |
| INMP441 **WS** | GPIO15 |
| INMP441 **SD** | GPIO34 |
| INMP441 **L/R** | GND |

Le micro se place devant la bouche, dans la mentonnière du casque, avec sa bonnette mousse.

## Les boutons

| Bouton | Mode voix (VOX, par défaut) | Mode « appuyer pour parler » |
| --- | --- | --- |
| A | Court : couper / rétablir le micro | Maintenir pour parler |
| B | Volume + | Volume + |
| C | Volume − | Volume − |

Le mode se choisit avec `MODE_APPUYER_POUR_PARLER` dans `config.h`.

## Mise en route

1. Copie `include/config.example.h` en `include/config.h` sur les deux intercoms. Garde **le même groupe et le même canal** des deux côtés.
2. Téléverse sur chaque carte avec `pio run -t upload`, puis ouvre `pio device monitor`.
3. Le moniteur affiche une ligne par seconde : niveau du micro, paquets envoyés, reçus et perdus.
4. Parle dans un micro : le niveau monte, « (émission) » apparaît et la LED bleue de l'autre carte s'allume.

## Régler le VOX

Regarde le niveau affiché en silence, puis en parlant normalement. Mets `SEUIL_VOX` entre les deux, plus près du silence. Sur la moto, le vent fera monter le niveau de repos : il faudra refaire le réglage en roulant (passager d'abord !).

## Points à vérifier ensemble

- **Niveau micro toujours à 0** : mets `MICRO_CANAL_DROIT` à 1. C'est une particularité connue de l'ESP32 avec l'INMP441.
- **Son trop faible ou saturé** : ajuste `GAIN_MICRO` (de 1 à 16).
- **Larsen** : si on entend un sifflement, baisse le volume ou éloigne le micro des haut-parleurs. Le VOX limite le problème.
- **Portée** : avec l'antenne intégrée, la portée est de l'ordre de la centaine de mètres en terrain dégagé, moins entre deux motos : c'est à mesurer. `LONGUE_PORTEE` à 1 sur les deux cartes active le mode longue portée d'Espressif ; l'antenne externe est dans le [bonus](../bonus).
- **Paquets perdus** : quelques-uns par minute ne s'entendent pas. Au-delà de 5 %, change de canal (1, 6 ou 11).
- **Plus de deux intercoms** : ils s'entendent tous, mais si deux personnes parlent en même temps leurs voix se mélangent mal. Pour l'instant, prévu pour deux.
