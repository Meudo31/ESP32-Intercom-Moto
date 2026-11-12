// Données propres à l'intercom échangées avec l'app : paquet DATA, JSON MEDIA et INFO, commandes.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>

static const size_t APP_DATA_MAX = 64;
// Remplit out avec le paquet DATA (octet 0 = version) ; renvoie sa taille.
size_t appDataBuild(uint8_t *out, size_t max);
// JSON du téléphone connecté et du morceau en cours.
std::string appMediaJson();
// JSON de la carte et du firmware.
std::string appInfoJson();
// Commande texte reçue du téléphone ("cle=valeur&...") : transmise à la tâche commandes.
void appCommand(const char *text, size_t len);
