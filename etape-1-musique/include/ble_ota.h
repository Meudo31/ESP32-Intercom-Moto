// ble_ota.h — mise à jour par BLE ; en cas d'échec l'ancien firmware reste en place.
#pragma once
#include <stddef.h>
#include <stdint.h>
#include <string>

bool bleOtaInProgress();

// Traite une commande texte de OTA_CTRL ; renvoie la réponse à notifier ("OK", "DONE", "ERR …").
std::string bleOtaControl(const std::string &cmd);
// Traite un morceau de OTA_DATA ; renvoie "" si tout va bien, sinon "ERR …".
std::string bleOtaData(const uint8_t *data, size_t len);
// Le téléphone s'est déconnecté : on annule un transfert en cours.
void bleOtaAbort();
// true quand la mise à jour est terminée et que l'ESP32 doit redémarrer.
bool bleOtaRestartPending();
