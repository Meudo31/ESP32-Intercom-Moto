#include "commandes.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include "audio.h"
#include "musique_bt.h"
#include "parametres.h"
#include "reglages.h"
#include "veille.h"

static QueueHandle_t s_file = nullptr;

bool commandeEnvoyer(const Commande &c) {
  return s_file && xQueueSend(s_file, &c, pdMS_TO_TICKS(100)) == pdTRUE;
}

bool commandeEnvoyer(Cmd type, int32_t valeur) {
  Commande c;
  c.type = type;
  c.valeur = valeur;
  return commandeEnvoyer(c);
}

static void changerVolume(int v) {
  audioSetVolume(v);
  reglagesSetVolume(audioVolume());
  Serial.printf("[Commande] Volume : %u %%\n", audioVolume());
}

static void executer(const Commande &c) {
  switch (c.type) {
    case Cmd::LECTURE:     musiqueLecture(); break;
    case Cmd::PAUSE:       musiquePause(); break;
    case Cmd::BASCULER:    musiqueBasculer(); break;
    case Cmd::STOP:        musiqueStop(); break;
    case Cmd::SUIVANT:     musiqueSuivant(); break;
    case Cmd::PRECEDENT:   musiquePrecedent(); break;
    case Cmd::VOLUME:      changerVolume(c.valeur); break;
    case Cmd::VOLUME_PLUS: changerVolume(audioVolume() + c.valeur); break;
    case Cmd::VOLUME_TEL:  musiqueSetVolumeTel(constrain(c.valeur, 0, 127)); break;
    case Cmd::VOLUME_TEL_PLUS:
      musiqueSetVolumeTel(constrain((int)musiqueVolumeTel() + c.valeur, 0, 127));
      Serial.printf("[Commande] Volume du téléphone : %u / 127\n", musiqueVolumeTel());
      break;
    case Cmd::VEILLE:      veilleProfonde(); break;
    case Cmd::GAIN:
      reglagesSetGain(ampliSetGain(c.valeur));
      break;
    case Cmd::VEILLE_AUTO:
      reglagesSetVeilleAuto(constrain(c.valeur, 0, 240));
      Serial.printf("[Commande] Veille auto : %u min\n", reglagesVeilleAuto());
      break;
    case Cmd::REDEMARRER:
      Serial.println("[Commande] Redémarrage");
      delay(300);   // laisse partir la réponse Bluetooth
      ESP.restart();
      break;
    case Cmd::NOM:
      if (c.texte[0]) {
        reglagesSetNom(c.texte);
        Serial.printf("[Commande] Nouveau nom « %s », redémarrage\n", c.texte);
        delay(300);
        ESP.restart();
      }
      break;
    case Cmd::DECONNECTER: musiqueDeconnecter(); break;
    case Cmd::RECONNECTER: musiqueReconnecter(); break;
    case Cmd::OUBLIER:     musiqueOublier(); break;
  }
}

static void tacheCommandes(void *) {
  Commande c;
  for (;;) {
    if (xQueueReceive(s_file, &c, portMAX_DELAY) == pdTRUE) executer(c);
  }
}

void commandesDemarrer() {
  s_file = xQueueCreate(10, sizeof(Commande));
  xTaskCreate(tacheCommandes, "commandes", COMMANDES_PILE, nullptr, COMMANDES_PRIO, nullptr);
}
