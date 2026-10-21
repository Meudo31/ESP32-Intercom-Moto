# Méthode de tournage : Bidouille Connectée

Valable pour tous les projets. Le plan détaillé de ce projet est dans [plan-de-tournage.md](plan-de-tournage.md).

## La réponse courte

**Tu tournes tout en une seule session par projet, en pensant d'abord à la vidéo longue, mais tu ne te contentes pas d'extraire les Shorts après coup.**

1. **Tu filmes la longue en 4K horizontal** (16:9). En gardant l'action au centre de l'image, tu pourras recadrer la plupart des plans de l'établi en vertical sans perte : un 4K recadré en 9:16 donne environ 1215 × 2160, plus que les 1080 × 1920 d'un Short.
2. **Tu doubles en vertical les 5 à 8 « moments forts »** (le résultat, la démo, le moment « waouh ») avec le téléphone tenu ou posé en vertical. Ce sont ces plans qui ouvrent les Shorts : une accroche filmée pour le vertical marche mieux qu'un recadrage.
3. **Tu montes la longue d'abord, puis les 5 Shorts** en piochant dans les mêmes rushes.

Extraire les Shorts uniquement de la longue donne des Shorts mous (cadrage serré, pas d'accroche). Tourner les Shorts séparément te ferait refaire le montage deux fois. Le mélange des deux te coûte 20 à 30 minutes de plus au tournage et règle les deux problèmes.

## Le matériel (minimum, rien d'obligatoire à acheter tout de suite)

| Rôle | Minimum | Mieux |
| --- | --- | --- |
| Caméra « établi » au-dessus du plan de travail | Téléphone en 4K sur un bras articulé ou un trépied avec bras horizontal (15 à 25 €) | Hybride ou caméra d'action |
| Caméra verticale | Second téléphone (même un ancien) ou le même téléphone déplacé | |
| Son | Micro-cravate filaire pour téléphone (10 à 20 €) | Micro-cravate sans fil type DJI Mic / Rode Wireless |
| Lumière | Une fenêtre de côté + une lampe de bureau LED | 2 panneaux LED à température réglable (30 à 50 € la paire) |
| Capture d'écran | OBS Studio (gratuit) pour VS Code et le terminal PlatformIO ; enregistreur d'écran du téléphone pour les notifications | |
| Montage | DaVinci Resolve (gratuit, gère les timelines verticales) pour la longue ; CapCut pour les sous-titres automatiques des Shorts | |

Pour les projets à LED (guirlande, matrice), baisse la luminosité des LED à 20-40 % et l'exposition de la caméra : sinon les LED « bavent » en taches blanches. Fond sombre, lumière de la pièce tamisée.

## Les règles de cadrage

- **Caméra établi** : pile au-dessus, les mains entrent par le bas de l'image. Tout ce qui compte (carte, câbles, composant) dans la **bande centrale** qui fait un tiers de la largeur : c'est ce qui restera en vertical.
- **Plans verticaux** : sujet au centre, rien d'important dans le cinquième du haut ni le quart du bas (l'interface YouTube les recouvre).
- **Chaque plan dure au moins 5 secondes** avant et après l'action : sans marge, impossible de monter.
- **Un clap des mains** au début de chaque prise quand deux caméras tournent ensemble : ça sert à les synchroniser au montage.

## Voix et visage

Pas besoin de montrer ton visage. Le plus simple et le plus efficace pour une chaîne d'électronique :

- **Pendant le tournage**, tu parles naturellement de ce que tu fais (son « direct »). Ça donne de la vie, même si tu en coupes la moitié.
- **Après le montage brut**, tu enregistres la **voix off** en lisant le script de la longue, au micro-cravate, dans une pièce avec des rideaux ou des meubles (pas une salle de bain). Tu réenregistres une phrase ratée tout de suite, sans tout reprendre.
- Les Shorts ont leur propre voix off de 30 à 60 s (texte dans chaque plan), enregistrée dans la même session.

Si un jour tu veux apparaître, une intro face caméra de 10 secondes suffit.

## Le déroulé d'une semaine (2 à 3 h par jour)

L'idée clé : **avoir une semaine d'avance**. La semaine où tu publies un projet, tu tournes le suivant. Les 3 premiers projets demandent donc 3 semaines de préparation avant la première sortie, ou une première semaine sans Shorts quotidiens.

| Jour | Tâche | Durée |
| --- | --- | --- |
| Lundi | Préparation : le montage marche déjà sur ta table (code flashé, testé). Relire le plan de tournage, sortir les pièces, charger batteries et téléphones, vider la mémoire, ranger l'établi. | 1 à 2 h |
| Mardi | **Tournage établi** : présentation des pièces, câblage étape par étape, captures d'écran (code, flash, téléphone). Caméra au-dessus + vertical sur les moments forts. | 2 à 3 h |
| Mercredi | **Tournage démo** : le projet en situation réelle (boîte aux lettres dehors, guirlande sur le sapin, casque sur la moto), plans d'ambiance, tous les plans verticaux restants. Voix off de la longue et des 5 Shorts. | 2 à 3 h |
| Jeudi | Montage de la longue : dérushage, montage brut sur la voix off, plans de coupe, musique, chapitres. | 3 h |
| Vendredi | Fin de la longue, miniature, puis les 5 Shorts (souvent 20 min chacun une fois les rushes triés) avec sous-titres. | 3 h |
| Week-end | Programmation des publications de la semaine suivante, réponses aux commentaires. | 30 min |

**Important** : tourne quand le projet **fonctionne déjà**. Le code de ces projets compile mais n'a pas été testé sur les cartes : fais la mise au point avant, hors caméra. Garde pourtant le téléphone qui filme pendant la mise au point : une panne, une fumée, un « pourquoi ça marche pas » font d'excellents Shorts.

## Publication

- La **longue** sort en début de semaine (par exemple le dimanche à 18 h ou le lundi).
- Les **5 Shorts** sortent un par jour du lundi au vendredi, à heure fixe (17-19 h). Chacun est relié à la longue avec la fonction **« Vidéo associée »** de YouTube Studio : c'est ce qui transforme les vues de Shorts en vues de la longue.
- Les Shorts sont republiés sur TikTok et Instagram Reels (sans le filigrane d'une autre plateforme).

## Rangement des fichiers

Un dossier par projet : `rushes/A-etabli`, `rushes/B-vertical`, `rushes/C-ecran`, `audio/voix-off`, `montage`, `exports`. Nomme les clips comme dans la liste de plans (`P07-reed-multimetre.mp4`) au moment du dérushage : tu retrouveras tout en un clin d'œil pour les Shorts.

## Légende des plans de tournage

- **H** : horizontal 4K, caméra établi ou trépied
- **V** : vertical, à tourner en plus (Shorts)
- **É** : capture d'écran (ordinateur ou téléphone)
- **★** : moment fort, à tourner **à la fois en H et en V**
