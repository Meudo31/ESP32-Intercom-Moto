# Script PlatformIO (pre:) : copie data/sonnerie.wav dans include/sonnerie_wav.h
# sous forme de tableau d'octets, pour l'intégrer au firmware.
# Pour changer de sonnerie : remplacer data/sonnerie.wav puis recompiler.
import os

Import("env")  # noqa: F821  (fourni par PlatformIO)

projet = env.subst("$PROJECT_DIR")  # noqa: F821
source = os.path.join(projet, "data", "sonnerie.wav")
cible = os.path.join(projet, "include", "sonnerie_wav.h")

with open(source, "rb") as f:
    octets = f.read()

lignes = [
    "// Fichier généré par outils/sonnerie_wav.py à partir de data/sonnerie.wav : ne pas modifier.",
    "#pragma once",
    "#include <stdint.h>",
    "#include <stddef.h>",
    "static const uint8_t SONNERIE_WAV[] = {",
]
for i in range(0, len(octets), 20):
    lignes.append("  " + ",".join(str(b) for b in octets[i:i + 20]) + ",")
lignes += ["};", "static const size_t SONNERIE_WAV_TAILLE = sizeof(SONNERIE_WAV);", ""]
texte = "\n".join(lignes)

ancien = None
if os.path.isfile(cible):
    with open(cible, "r", encoding="utf-8") as f:
        ancien = f.read()
if ancien != texte:
    with open(cible, "w", encoding="utf-8") as f:
        f.write(texte)
    print("Sonnerie : %s (%d octets) intégrée" % (source, len(octets)))
