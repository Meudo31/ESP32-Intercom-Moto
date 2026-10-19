# Bonus : pour aller plus loin

Idées pour une vidéo de suite, une fois l'étape 3 au point. Pas de code pour l'instant.

## Antenne externe

L'ESP32-WROOM-32**U** a un connecteur IPEX à la place de l'antenne gravée. Avec une petite antenne 2,4 GHz collée sous la coque du boîtier, la portée entre deux motos peut être nettement meilleure. Le code ne change pas. À combiner avec `LONGUE_PORTEE` de l'étape 2 pour un test de portée filmé.

## Stéréo

Un second MAX98357A par casque, avec sa broche SD réglée sur le canal droit (le premier sur le gauche), donne une vraie stéréo pour la musique. Il faut alors envoyer la musique en stéréo au lieu de mono dans le code de l'étape 3.

## Télécommande au guidon

Les boutons sur le casque sont pratiques mais pas toujours faciles à trouver avec des gants. Un petit ESP32-C3 avec trois boutons au guidon, qui envoie les commandes en ESP-NOW à l'intercom, éviterait de lâcher le guidon.

## Groupe de plus de deux motos

Pour trois motos ou plus, il faudrait un tampon par émetteur (repéré par son adresse MAC) et un mélange des voix, comme le fait déjà l'étape 3 entre musique et voix.

## Appels téléphoniques

Répondre au téléphone dans le casque demande le profil Bluetooth « mains libres » (HFP), que la bibliothèque utilisée ici ne gère pas. C'est possible avec l'ESP-IDF d'Espressif, mais c'est un projet à part entière.
