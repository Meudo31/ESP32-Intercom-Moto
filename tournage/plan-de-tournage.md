# Plan de tournage n°3 : l'intercom moto

Méthode générale et légende (H, V, É, ★) : voir [methode-de-tournage.md](methode-de-tournage.md).

## L'essentiel

- **Titre proposé** : « J'ai fabriqué mon intercom moto pour 50 € (ESP32, sans réseau) »
- **Promesse** : la musique du téléphone et la voix entre pilote et passager, sans abonnement ni appli, pour le prix d'un plein.
- **Durée visée** : 12 à 15 min (le projet le plus riche). Les 3 étapes deviennent 3 chapitres.
- **Besoin d'aide ?** **Oui** : il faut une deuxième personne pour la démo de l'intercom (passager ou second motard). C'est aussi la seule vidéo où un visage, même casqué, aide beaucoup : la moto et le casque *sont* le décor.
- **À avoir prêt avant de tourner** : les deux boîtiers de l'étape 3 qui marchent sur batterie, ton casque et celui du passager, la Scout Bobber propre, une caméra d'action (ou un téléphone) avec fixation, un micro-cravate de plus à glisser dans le casque, 2 ou 3 musiques **libres de droits**.

## Sécurité et règles pendant le tournage

- **Aucune manipulation de téléphone ou de caméra en roulant.** Tout se règle à l'arrêt ; les plans en roulant sont filmés par une caméra fixée (casque, réservoir, guidon) ou par une autre personne depuis le bord de la route.
- Route calme, de jour, sans passager inutilement exposé. Rappelle en voix off la règle française : haut-parleurs intégrés au casque autorisés, écouteurs interdits, volume qui laisse entendre la circulation.
- Pas de plaque d'immatriculation lisible, ni la tienne ni celle des autres (floutage au montage).

## Comment filmer le son dans le casque

C'est le défi de cette vidéo : le spectateur doit **entendre ce que tu entends**.

1. Glisse un **micro-cravate dans le casque, contre le haut-parleur**, relié à un téléphone ou un petit enregistreur dans ta poche. C'est la piste « dans le casque ».
2. Le micro de la caméra d'action enregistre le bruit du moteur et du vent : c'est la piste « extérieur ».
3. Au montage, passe de l'une à l'autre (effet « on entre dans le casque »), avec une petite icône casque à l'écran.
4. Pour la démo à l'arrêt, plus simple : caméra devant les deux casques posés côte à côte, tu parles dans l'un et on entend l'autre.

## Script de la longue

| Temps | Chapitre | Ce qu'on voit | Ce que tu dis (voix off, à adapter) |
| --- | --- | --- | --- |
| 0:00 | Accroche | P01, P02 | « Un intercom moto du commerce, c'est 200 à 400 euros la paire. Celui-ci m'a coûté 50 euros, et il marche sans téléphone ni réseau. » |
| 0:25 | Le projet | P03, P04 | Les 3 étapes : la musique, la voix, les deux ensemble sur batterie. Pourquoi un ESP32 classique (Bluetooth classique obligatoire). |
| 1:00 | Le matériel | P05, P06 | ESP32-WROOM-32, MAX98357A, haut-parleurs fins, boutons étanches utilisables avec des gants, micros INMP441. |
| 2:30 | Étape 1 : la musique | P07 à P13 | Le câblage I2S, l’appairage Bluetooth, les boutons, le premier son. |
| 5:00 | Étape 2 : l'intercom | P14 à P20 | ESP-NOW expliqué simplement (« les deux ESP32 se parlent directement, comme des talkies-walkies »), le micro, le déclenchement à la voix, le premier « tu m'entends ? ». |
| 8:00 | Étape 3 : les deux ensemble | P21 à P25 | La musique qui baisse quand l'autre parle, la version sur batterie, le boîtier. |
| 10:00 | Installation sur le casque | P26 à P28 | Haut-parleurs dans les mousses, micro et bonnette, boîtier au velcro. |
| 11:00 | Essai sur la route | P29 à P33 | Pilote et passager, la musique, la conversation, la portée entre deux motos si possible. |
| 13:00 | Bilan honnête | P34 | Qualité de son, autonomie, portée, ce qui reste à améliorer, comparaison avec un intercom du commerce. |
| 14:00 | Fin | P35 | Teaser du bonus (antenne externe, stéréo, télécommande au guidon), appel à l'abonnement. |

## Liste des plans

| N° | Plan | Type | Note |
| --- | --- | --- | --- |
| P01 | Moto qui passe sur une belle route, son du moteur | ★ H+V | Filmé depuis le bord de route ou par drone si tu l'as |
| P02 | Le casque posé sur la selle, le boîtier fixé dessus, travelling lent | ★ H+V | Lumière de fin de journée |
| P03 | Les deux boîtiers finis posés côte à côte | H | |
| P04 | Intercom du commerce (photo ou le tien si tu en as un) à côté du tien | H | Pour la comparaison de prix |
| P05 | Toutes les pièces posées une à une | H | |
| P06 | Gros plan haut-parleur fin, épaisseur mesurée au pied à coulisse | ★ H+V | Montre qu'il rentre dans un casque |
| P07 | Câblage ESP32 + MAX98357A en accéléré | H | |
| P08 | Câblage des 3 boutons | H | |
| P09 | `config.h` : `NOM_BLUETOOTH`, `VOLUME_DEPART` | É | |
| P10 | Téléversement | É | |
| P11 | Le téléphone trouve l’intercom dans le Bluetooth (nom réglé par `NOM_BLUETOOTH`), appairage | ★ H+V | Téléphone et carte dans le même cadre, la LED bleue s'allume |
| P12 | Premier son dans les haut-parleurs posés sur l'établi | ★ H+V | Réaction |
| P13 | Les boutons avec des gants de moto : lecture, piste suivante, volume | ★ H+V | Le plan qui prouve que c'est pensé pour la moto |
| P14 | Micro INMP441 en gros plan, avec et sans bonnette | H | |
| P15 | Câblage du micro | H | |
| P16 | Schéma ESP-NOW animé ou dessiné à la main | H | Une feuille et un feutre filmés au-dessus suffisent |
| P17 | Les deux montages sur l'établi à 2 mètres, tu parles dans l'un, l'autre restitue | ★ H+V | Le moment « waouh » de l'étape 2 |
| P18 | Démo du déclenchement à la voix : silence, puis ta voix ouvre le micro (LED ou moniteur série) | H | |
| P19 | Test de portée : ton complice s'éloigne dans la rue ou le jardin en parlant | ★ H+V | Compte les pas ou mesure la distance |
| P20 | Moniteur série : paquets envoyés et reçus | É | |
| P21 | Musique dans un casque, l'autre parle, la musique baisse puis remonte | ★ H+V | Micro-cravate dans le casque, le plan clé de la vidéo |
| P22 | Batterie 18650, TP4056, MT3608 : câblage | H | |
| P23 | Réglage du MT3608 à 5 V au multimètre | H | |
| P24 | Tout rentre dans le boîtier étanche, fermeture | ★ H+V | Accéléré satisfaisant |
| P25 | Recharge en USB-C, LED du TP4056 | H | |
| P26 | Haut-parleurs glissés dans les mousses du casque | ★ H+V | |
| P27 | Micro collé près de la bouche avec la bonnette | H | |
| P28 | Boîtier fixé au velcro, câble jack rangé | H | |
| P29 | Départ : casque enfilé, contact, on démarre | H+V | Filmé par le complice |
| P30 | En roulant, caméra embarquée, piste « dans le casque » : la musique | ★ H+V | Caméra fixée, jamais tenue |
| P31 | En roulant : la conversation pilote / passager | ★ H+V | Sous-titres obligatoires au montage |
| P32 | Deux motos en convoi : portée qui décroche, puis revient | H | Seulement si tu as un second motard |
| P33 | Arrêt, casque retiré, première réaction | H+V | |
| P34 | Face caméra ou voix off sur des plans de la moto à l'arrêt : le bilan | H | |
| P35 | Moto qui s'éloigne au coucher du soleil | H | Plan de fin |

## Les 5 Shorts

| N° | Accroche (2 premières secondes) | Plans | Déroulé |
| --- | --- | --- | --- |
| S1 | « Mon intercom moto m'a coûté 50 €, pas 300 » | P01, P02, P31, P24 | Le Short vitrine : la route, le casque, la conversation sous-titrée. À publier en premier. |
| S2 | « Il fait baisser la musique quand ton passager parle » | P21, P30 | Le son « dans le casque » : musique, voix, musique qui remonte. Ton Short le plus partageable. |
| S3 | « Deux ESP32 qui se parlent sans Wi-Fi ni réseau » | P17, P16, P19 | ESP-NOW expliqué en 40 s avec le test de portée. Short pédagogique, intéressera aussi les non-motards. |
| S4 | « Des boutons qu'on peut utiliser avec des gants » | P13, P06, P26 | Pensé pour la moto : boutons, haut-parleurs fins, installation dans le casque. |
| S5 | « Tout un intercom dans cette petite boîte » | P24, P22, P25, P28 | Accéléré du montage dans le boîtier étanche, en musique. Short court et satisfaisant. |

## Checklist du jour J

- Les deux boîtiers chargés à 100 %, batteries de téléphone et de caméra pleines, cartes mémoire vides.
- Le complice briefé sur les plans P19, P29 et P31-P33 (imprime-lui la liste).
- Micro-cravate testé dans le casque **avant** de partir : écoute la prise sur 10 secondes en roulant à basse vitesse.
- Musique libre de droits sur le téléphone (une musique du commerce rendrait la vidéo inutilisable).
- Météo sèche : les boîtiers sont étanches, pas forcément les montages de test.
