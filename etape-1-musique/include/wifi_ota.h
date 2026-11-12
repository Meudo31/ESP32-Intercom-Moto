// Mise à jour du firmware par Wi-Fi : l'intercom ouvre un point d'accès Wi-Fi temporaire,
// le téléphone s'y connecte et envoie le .bin en HTTP (beaucoup plus rapide que le BLE).
// Démarrée par l'app via BLE (commande OTA_CTRL « WIFI <clé> »).
#pragma once
#include <string>

// Démarre le point d'accès et le serveur. Renvoie "WIFI <ssid> <mot de passe>" ou "ERR …".
std::string wifiOtaDemarrer();
bool wifiOtaActif();
