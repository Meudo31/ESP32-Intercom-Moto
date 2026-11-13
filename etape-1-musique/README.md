# Étape 1 : la musique Bluetooth dans le casque

L'ESP32 se présente au téléphone comme une enceinte Bluetooth nommée « Intercom Moto ». La musique est envoyée en numérique (I2S) à l'ampli MAX98357A, qui alimente les deux haut-parleurs du casque. Un seul bouton par défaut, qui fait tout grâce à des combinaisons d'appuis (lecture, pistes, volume, veille). Deux boutons de plus peuvent être ajoutés en option.

Carte conseillée : **D1 mini ESP32** (module WROOM-32, environ 39 × 31 mm). Toute carte ESP32 classique convient ; pas d'ESP32-S3, C3, C6 ni P4, qui n'ont pas de Bluetooth classique.

Le code compile avec PlatformIO (ESP32-WROOM-32, framework Arduino, bibliothèque ESP32-A2DP 1.8.7, organisé en tâches FreeRTOS, un module par élément) mais n'a pas encore été testé sur une vraie carte.

## Câblage

| De | Vers |
| --- | --- |
| MAX98357A **VIN** | **5V** (ou VIN) de l'ESP32 |
| MAX98357A **GND** | GND |
| MAX98357A **BCLK** | GPIO27 |
| MAX98357A **LRC** | GPIO26 |
| MAX98357A **DIN** | GPIO14 |
| MAX98357A **SD** | GPIO13 (le firmware coupe l'ampli pendant la veille profonde) |
| MAX98357A **GAIN** | GPIO12 : gain réglable depuis l'app (6, 9 ou 12 dB). Attention, voir « Gain sur GPIO12 » ci-dessous |
| MAX98357A sorties **+** et **−** | les deux haut-parleurs **en parallèle** (+ avec +, − avec −) |
| Bouton A | GPIO15 et GND |

Ce brochage convient à la DevKit V1 et à la D1 mini ESP32 ; la LED bleue de la carte est sur GPIO2. Si on appuie sur le bouton pendant la mise sous tension, l'ESP32 démarre normalement mais n'affiche pas ses premiers messages sur le port série (GPIO15 est une broche de démarrage) : sans conséquence.

Deux haut-parleurs 8 Ω en parallèle font 4 Ω, ce que le MAX98357A accepte. Pendant les essais, la carte est alimentée par son port USB.

### Gain sur GPIO12

GPIO12 est une broche lue au démarrage de l'ESP32 : si elle est au niveau haut à la mise sous tension, l'ESP32 alimente sa mémoire flash en 1,8 V au lieu de 3,3 V. Le module WROOM-32 ne démarre alors plus : rien sur le port série, ou des erreurs de flash en boucle, et le flash par USB échoue. Or la broche GAIN du MAX98357A, laissée libre par l'ampli, peut tirer GPIO12 vers le haut.

Si ça arrive, deux solutions :

1. **Débrancher le fil GAIN** pendant la mise sous tension et le flash, puis le rebrancher.
2. **Fixer une fois pour toutes la flash en 3,3 V** par eFuse : `espefuse.py --port COMx set_flash_voltage 3.3V`. GPIO12 n'est alors plus lu au démarrage. C'est **définitif et irréversible**, mais sans risque pour un WROOM-32 (sa flash est en 3,3 V). Ne pas le faire sur un module WROVER à flash 1,8 V.

## Le bouton A

Un seul bouton (entre GPIO15 et GND) fait tout :

| Geste | Action |
| --- | --- |
| Appui court | Lecture / pause |
| Deux appuis courts | Morceau suivant |
| Appui court puis appui long | Morceau précédent |
| Appui long (plus de 0,6 s) | Volume + |
| Appui long puis appui court | Volume − |
| Appui très long (3 s, sans relâcher) | Veille profonde |

Pour enchaîner deux appuis, il faut moins de 0,4 s entre les deux ; c'est aussi le petit délai avant qu'un appui seul soit pris en compte. Le volume réglé est celui du téléphone (environ 8 % par appui). Les durées se règlent dans `config.h` (`GESTE_LONG_MS`, `GESTE_TRES_LONG_MS`, `GESTE_ATTENTE_MS`, `PAS_VOLUME_BOUTON`).

### Boutons B et C (option)

Pour les ajouter, mets `AVEC_BOUTONS` à 1 dans `config.h` et câble B sur GPIO33 et GND, C sur GPIO4 et GND.

| Bouton | Appui court | Appui long |
| --- | --- | --- |
| B | Volume de l'intercom + | Morceau suivant |
| C | Volume de l'intercom − | Morceau précédent |

## Veille profonde

Un appui très long de 3 secondes sur A éteint presque tout : la LED s'éteint et le téléphone voit l'intercom se déconnecter. Un appui sur A le réveille ; il redémarre et se reconnecte tout seul au téléphone en quelques secondes.

En veille, l'ESP32 ne consomme que quelques µA, mais la carte et l'ampli continuent de consommer un peu : le régulateur et la LED rouge d'alimentation de la D1 mini, alors que le MAX98357A est coupé par sa broche SD. Pour un rangement de plusieurs jours, garde un vrai interrupteur sur la batterie.

## L'app Android

L'app **Intercom Moto** (dossier [`app-android`](../app-android)) se connecte à l'intercom en Bluetooth basse consommation (BLE), en plus de la musique. Elle affiche :

- le téléphone connecté pour la musique, la lecture en cours, la fréquence, le débit et la qualité du signal ;
- le morceau (titre, artiste, album, piste, position) ;
- la carte : firmware, puce, mémoire, temps de marche, raison du dernier démarrage.

Et elle commande tout : lecture / pause / stop / pistes, volume de l'intercom et du téléphone, déconnecter / reconnecter / oublier le téléphone, renommer l'intercom, veille automatique, veille profonde, redémarrage, et **mise à jour du firmware** sans câble.

La clé de mise à jour est `OTA_KEY` dans `config.h` (`intercom-moto` par défaut).

**Important** : à partir de la version 1.1.0, la table des partitions change (deux emplacements pour la mise à jour). Le premier flash de cette version se fait **par USB** (`pio run -t upload`), ensuite tout passe par l'app.

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
