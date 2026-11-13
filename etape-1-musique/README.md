# Étape 1 : la musique et les appels Bluetooth dans le casque

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
| Bouton A | GPIO4 et GPIO2 (le firmware met GPIO2 à GND et l'y maintient pendant la veille) |
| Micro INMP441 **VDD** | **3V3** (surtout pas 5 V) |
| Micro INMP441 **GND** | GND |
| Micro INMP441 **L/R** | GPIO33 (mise à GND par le firmware : le micro parle sur la voie gauche) |
| Micro INMP441 **SCK** | GPIO25 |
| Micro INMP441 **WS** | GPIO32 |
| Micro INMP441 **SD** | GPIO35 |

Ce brochage convient à la DevKit V1 et à la D1 mini ESP32. GPIO2 servant de masse au bouton, la LED bleue de la carte (sur GPIO2) n'indique plus l'état : elle reste éteinte (`BROCHE_LED` à -1). Pour relier le bouton directement à GND, mets `BROCHE_BOUTON_A_MASSE` à -1. GPIO2 est une broche de démarrage, mais le firmware ne la met à GND qu'une fois démarré : le flash par USB n'est pas gêné.

Deux haut-parleurs 8 Ω en parallèle font 4 Ω, ce que le MAX98357A accepte. Pendant les essais, la carte est alimentée par son port USB.

### Gain sur GPIO12

GPIO12 est une broche lue au démarrage de l'ESP32 : si elle est au niveau haut à la mise sous tension, l'ESP32 alimente sa mémoire flash en 1,8 V au lieu de 3,3 V. Le module WROOM-32 ne démarre alors plus : rien sur le port série, ou des erreurs de flash en boucle, et le flash par USB échoue. Or la broche GAIN du MAX98357A, laissée libre par l'ampli, peut tirer GPIO12 vers le haut.

Si ça arrive, deux solutions :

1. **Débrancher le fil GAIN** pendant la mise sous tension et le flash, puis le rebrancher.
2. **Fixer une fois pour toutes la flash en 3,3 V** par eFuse : `espefuse.py --port COMx set_flash_voltage 3.3V`. GPIO12 n'est alors plus lu au démarrage. C'est **définitif et irréversible**, mais sans risque pour un WROOM-32 (sa flash est en 3,3 V). Ne pas le faire sur un module WROVER à flash 1,8 V.

## Les appels (depuis le firmware 2.0.0)

L'intercom est aussi un kit mains libres (profil Bluetooth HFP) : quand le téléphone reçoit un appel, une sonnerie joue en boucle dans le casque et le bouton A sert à répondre.

| Moment | Geste sur A | Action |
| --- | --- | --- |
| Ça sonne | Appui court | Décrocher |
| Ça sonne | Deux appuis courts | Rejeter |
| En appel | Appui court | Raccrocher |
| En appel | Appui long / long puis court | Volume de l'appel + / − |

Hors appel, le bouton garde ses gestes habituels (musique, volume, veille). Si une LED est configurée (`BROCHE_LED`), elle clignote vite pendant la sonnerie et reste allumée pendant l'appel.

**Il faut un micro** pour que l'interlocuteur t'entende : le haut-parleur seul ne suffit pas. Le firmware attend un **INMP441** (micro numérique I2S, environ 3 €), câblé comme dans le tableau ci-dessus. Les broches 25, 32, 33 et 35 ne jouent aucun rôle au démarrage de l'ESP32 et sont libres sur la DevKit V1 comme sur la D1 mini ESP32. On peut aussi relier L/R directement à GND et mettre `BROCHE_MICRO_LR` à -1. Sans micro, mets `AVEC_MICRO` à 0 dans `config.h` : tu entendras l'appel, l'autre n'entendra que du silence. Si l'autre t'entend trop faiblement ou trop fort, règle `GAIN_MICRO` (1 à 32, 4 par défaut).

À savoir :
- **Le téléphone doit voir les « appels »** : s'il avait été appairé avec une version plus ancienne, oublie « Intercom Moto » dans son Bluetooth puis appaire-le de nouveau, et vérifie que l'option « Appels téléphoniques » est cochée.
- Le son des appels est en qualité téléphone (8 kHz). Il n'y a pas d'annulation d'écho : garde le micro près de la bouche et loin des haut-parleurs (devant la mentonnière, par exemple), sinon l'interlocuteur peut s'entendre en écho.
- Le volume de l'appel se règle aussi avec les touches de volume du téléphone pendant l'appel.

### Changer la sonnerie

La sonnerie est le fichier [`data/sonnerie.wav`](data/sonnerie.wav) (un petit carillon créé pour ce projet, libre de droits). Pour en mettre une autre, remplace ce fichier par le tien, **sous le même nom**, puis recompile : le fichier est intégré au firmware. Formats acceptés : WAV PCM 8 ou 16 bits, mono ou stéréo, de 8 000 à 48 000 Hz. Garde-le court et léger (2 secondes en 16 000 Hz mono 16 bits font 64 Ko ; il reste environ 250 Ko de place). Avec Audacity : *Pistes > Mixer > stéréo vers mono*, fréquence du projet 16000 Hz, *Exporter en WAV 16 bits PCM signé*. Le silence entre deux sonneries (`SONNERIE_PAUSE_MS`) et le volume (`SONNERIE_VOLUME`) se règlent dans `config.h`.

### Pourquoi la compilation est différente

Le noyau Arduino précompilé pour l'ESP32 envoie la voix des appels sur des broches « PCM » destinées à une puce audio externe, au lieu de la donner au programme. Depuis la version 2.0.0, le projet compile donc le noyau Arduino comme composant d'ESP-IDF (`framework = arduino, espidf` dans `platformio.ini`), avec la configuration de [`sdkconfig.defaults`](sdkconfig.defaults) : c'est celle d'Arduino à l'identique, sauf quelques lignes marquées « Intercom ». La première compilation prend 5 à 10 minutes. La table de partitions ne change pas : la mise à jour par l'app depuis la version 1.5.1 fonctionne.

## Le bouton A

Un seul bouton (entre GPIO4 et GPIO2) fait tout. Hors appel :

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

Pour les ajouter, mets `AVEC_BOUTONS` à 1 dans `config.h` et câble B sur GPIO16 et GND, C sur GPIO17 et GND.

| Bouton | Appui court | Appui long |
| --- | --- | --- |
| B | Volume de l'intercom + | Morceau suivant |
| C | Volume de l'intercom − | Morceau précédent |

## Veille profonde

Un appui très long de 3 secondes sur A éteint presque tout : le téléphone voit l'intercom se déconnecter. Un appui sur A le réveille ; il redémarre et se reconnecte tout seul au téléphone en quelques secondes.

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
3. Sur le téléphone, cherche « Intercom Moto » dans le Bluetooth et appaire-le.
4. Lance une musique : elle sort dans les haut-parleurs. Règle le volume depuis le téléphone (ou avec les boutons si tu les as ajoutés).

La carte se reconnecte toute seule au dernier téléphone appairé au démarrage suivant.

## Points à vérifier ensemble

- **Son trop faible** : relie la broche GAIN du MAX98357A à GND (12 dB) ou à GND par une résistance de 100 kΩ (15 dB).
- **Grésillement** : souvent l'alimentation USB du PC. Essaie un chargeur de téléphone, et garde les fils I2S courts.
- **Fréquence** : le moniteur série affiche la fréquence choisie par le téléphone (44 100 ou 48 000 Hz). Les deux fonctionnent.
