# Intercom moto

Un intercom pour casque moto fait maison : la musique du téléphone en Bluetooth, et la voix entre deux casques (pilote et passager, ou deux motos en convoi) sans téléphone ni réseau, grâce à ESP-NOW. Le projet avance par étapes, du plus simple au plus complet ; chaque étape est un projet PlatformIO autonome et peut faire l'objet d'une vidéo.

| Étape | Dossier | Ce que ça fait | Cartes |
| --- | --- | --- | --- |
| 1 | [etape-1-musique](etape-1-musique) | Lecteur de musique Bluetooth dans le casque, boutons lecture / pistes / volume | 1 × ESP32 |
| 2 | [etape-2-intercom](etape-2-intercom) | Intercom vocal entre deux casques en ESP-NOW, déclenché à la voix ou au bouton | 2 × ESP32 |
| 3 | [etape-3-musique-et-intercom](etape-3-musique-et-intercom) | Les deux en même temps : la musique baisse quand l'autre parle. Version sur batterie | 2 × ESP32 |
| Bonus | [bonus](bonus) | Antenne externe, stéréo, télécommande au guidon, appels téléphoniques | |

**Carte obligatoire : ESP32 « classique » (ESP32-WROOM-32).** Les ESP32-S3, C3 et C6 n'ont pas de Bluetooth classique et ne peuvent pas recevoir de musique.

Le câblage est le même aux trois étapes : on ajoute des pièces, on ne refait rien.

Les trois étapes compilent avec PlatformIO mais n'ont pas encore été testées sur de vraies cartes.

## Liste d'achat par étape

Prix approximatifs ; chaque étape réutilise ce qui a été acheté avant. Les liens sont des recherches, pour comparer les vendeurs.

### Étape 1 : un boîtier (environ 20 à 30 €)

| Pièce | Prix approx. | Recherche |
| --- | --- | --- |
| ESP32-WROOM-32 DevKitC (38 broches, USB-C de préférence) | 6 à 8 € | [Amazon](https://www.amazon.fr/s?k=ESP32+WROOM-32+DevKitC) · [AliExpress](https://fr.aliexpress.com/w/wholesale-ESP32-WROOM-32-DevKitC.html) |
| Ampli I2S MAX98357A | 2 à 4 € | [Amazon](https://www.amazon.fr/s?k=MAX98357A) · [AliExpress](https://fr.aliexpress.com/w/wholesale-MAX98357A.html) |
| 2 haut-parleurs fins 40 mm, 8 Ω, 2 à 3 W (à glisser dans les mousses du casque) | 3 à 6 € | [Amazon](https://www.amazon.fr/s?k=haut-parleur+40mm+8+ohm+fin) · [AliExpress](https://fr.aliexpress.com/w/wholesale-speaker-40mm-8ohm-thin.html) |
| 3 boutons poussoirs étanches 16 mm (gros, utilisables avec des gants) | 3 à 6 € | [Amazon](https://www.amazon.fr/s?k=bouton+poussoir+16mm+%C3%A9tanche) · [AliExpress](https://fr.aliexpress.com/w/wholesale-16mm-waterproof-push-button.html) |
| Plaque d'essai + fils Dupont | 4 à 6 € | [Amazon](https://www.amazon.fr/s?k=breadboard+fils+dupont) |

### Étape 2 : le second casque et les micros (environ 25 à 35 € en plus)

| Pièce | Prix approx. | Recherche |
| --- | --- | --- |
| Second ESP32-WROOM-32, MAX98357A, 2 haut-parleurs, 3 boutons (comme l'étape 1) | 15 à 25 € | |
| 2 micros numériques I2S INMP441 (souvent vendus par 3) | 5 à 8 € | [Amazon](https://www.amazon.fr/s?k=INMP441) · [AliExpress](https://fr.aliexpress.com/w/wholesale-INMP441.html) |
| Bonnettes mousse pour micro (contre le vent) | 2 à 3 € | [Amazon](https://www.amazon.fr/s?k=bonnette+mousse+micro+cravate) |

### Étape 3 : version embarquée sur batterie (environ 30 à 40 € pour les deux boîtiers)

Pas de nouvelle pièce électronique pour le code ; ces pièces servent à rendre les deux intercoms autonomes. Quantités pour **deux** boîtiers.

| Pièce | Prix approx. | Recherche |
| --- | --- | --- |
| 2 batteries 18650 de marque (Samsung, LG, Molicel) + 2 supports | 10 à 14 € | [Amazon](https://www.amazon.fr/s?k=18650+samsung+35E) |
| 2 modules de charge TP4056 avec protection (6 broches, USB-C) | 2 € | [Amazon](https://www.amazon.fr/s?k=TP4056+USB-C+protection) · [AliExpress](https://fr.aliexpress.com/w/wholesale-TP4056-type-c-protection.html) |
| 2 convertisseurs élévateurs MT3608 (réglés à 5 V) | 2 € | [Amazon](https://www.amazon.fr/s?k=MT3608) · [AliExpress](https://fr.aliexpress.com/w/wholesale-MT3608.html) |
| 2 interrupteurs à glissière ou à bascule | 1 à 2 € | |
| 2 petits boîtiers étanches (environ 80 × 50 × 25 mm) | 6 à 10 € | [Amazon](https://www.amazon.fr/s?k=bo%C3%AEtier+%C3%A9tanche+%C3%A9lectronique+petit) |
| 2 rallonges jack 3,5 mm 4 pôles (casque débranchable du boîtier) | 4 à 6 € | [Amazon](https://www.amazon.fr/s?k=rallonge+jack+3.5+4+p%C3%B4les) |
| Velcro adhésif ou attache casque | 2 à 4 € | |

### Bonus

| Pièce | Prix approx. | Recherche |
| --- | --- | --- |
| ESP32-WROOM-32**U** + antenne 2,4 GHz IPEX (meilleure portée entre deux motos) | 8 à 12 € pièce | [AliExpress](https://fr.aliexpress.com/w/wholesale-ESP32-WROOM-32U-antenna.html) |
| Second MAX98357A par casque (stéréo) | 2 à 4 € | |

## Mise en route

Chaque dossier a son propre README avec le câblage et la mise en route. Dans tous les cas : copier `include/config.example.h` en `include/config.h`, adapter les valeurs, puis `pio run -t upload` depuis le dossier de l'étape. Le fichier `config.h` est exclu du dépôt.

## Sécurité et réglementation

En France, rouler avec des écouteurs ou des oreillettes est interdit depuis 2015, mais les haut-parleurs intégrés au casque restent autorisés : c'est le cas de cet intercom. Garde un volume qui laisse entendre la circulation, et vérifie la réglementation de ton pays avant de rouler avec.
