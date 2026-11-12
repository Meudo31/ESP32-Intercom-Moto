#include "boutons_task.h"

#include <Arduino.h>
#include "bouton.h"
#include "commandes.h"
#include "parametres.h"

static void tacheBoutons(void *) {
  Bouton a(BROCHE_BOUTON_A, 2000);  // appui long de 2 s pour éviter une veille par erreur
  a.begin();
#if AVEC_BOUTONS
  Bouton b(BROCHE_BOUTON_B), c(BROCHE_BOUTON_C);
  b.begin();
  c.begin();
#endif
  for (;;) {
    switch (a.lire()) {
      case Appui::COURT: commandeEnvoyer(Cmd::BASCULER); break;
      case Appui::LONG:  commandeEnvoyer(Cmd::VEILLE); break;
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
