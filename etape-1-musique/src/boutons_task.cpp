#include "boutons_task.h"

#include <Arduino.h>
#include "bouton.h"
#include "commandes.h"
#include "geste.h"
#include "parametres.h"

static void tacheBoutons(void *) {
  BoutonGestes a(BROCHE_BOUTON_A, GESTE_LONG_MS, GESTE_TRES_LONG_MS, GESTE_ATTENTE_MS);
  a.begin();
#if AVEC_BOUTONS
  Bouton b(BROCHE_BOUTON_B), c(BROCHE_BOUTON_C);
  b.begin();
  c.begin();
#endif
  for (;;) {
    switch (a.lire()) {
      case Geste::COURT:      commandeEnvoyer(Cmd::BASCULER); break;
      case Geste::DOUBLE:     commandeEnvoyer(Cmd::SUIVANT); break;
      case Geste::COURT_LONG: commandeEnvoyer(Cmd::PRECEDENT); break;
      case Geste::LONG:       commandeEnvoyer(Cmd::VOLUME_TEL_PLUS, PAS_VOLUME_BOUTON); break;
      case Geste::LONG_LONG:  commandeEnvoyer(Cmd::VOLUME_TEL_PLUS, 2 * PAS_VOLUME_BOUTON); break;
      case Geste::LONG_COURT: commandeEnvoyer(Cmd::VOLUME_TEL_PLUS, -PAS_VOLUME_BOUTON); break;
      case Geste::TRES_LONG:  commandeEnvoyer(Cmd::VEILLE); break;
      default: break;
    }
#if AVEC_BOUTONS
    switch (b.lire()) {
      case Appui::COURT: commandeEnvoyer(Cmd::VOLUME_PLUS, 10); break;
      case Appui::LONG:  commandeEnvoyer(Cmd::SUIVANT); break;
      default: break;
    }
    switch (c.lire()) {
      case Appui::COURT: commandeEnvoyer(Cmd::VOLUME_PLUS, -10); break;
      case Appui::LONG:  commandeEnvoyer(Cmd::PRECEDENT); break;
      default: break;
    }
#endif
    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

void boutonsDemarrer() {
  xTaskCreate(tacheBoutons, "boutons", BOUTONS_PILE, nullptr, BOUTONS_PRIO, nullptr);
}
