// Veille profonde (réveil par le bouton A) et veille automatique sans connexion.
#pragma once

void veilleProfonde();          // ne revient pas
void veilleAuReveil();          // à appeler au démarrage
void surveillanceDemarrer();    // tâche : signal Bluetooth + veille automatique
