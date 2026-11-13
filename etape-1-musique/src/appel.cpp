#include "appel.h"

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "esp_hf_client_api.h"
#include "audio.h"
#include "musique_bt.h"
#include "parametres.h"
#include "sonnerie.h"
#include "system_events.h"
#include "voix.h"

// Mis à jour par la pile Bluetooth (sa propre tâche), lus par la tâche « appel ».
static volatile bool s_slc = false;           // liaison mains libres établie
static volatile uint8_t s_appel = 0;          // indicateur « call » : 1 = appel en cours
static volatile uint8_t s_preparation = 0;    // indicateur « callsetup » : 1 entrant, 2-3 sortant
static volatile uint32_t s_audio = 0;         // 0 = pas de liaison audio, sinon sa fréquence
static volatile uint8_t s_volume = VOLUME_APPEL_DEPART;
static volatile EtatAppel s_etat = EtatAppel::AUCUN;
static char s_numero[24] = "";
static portMUX_TYPE s_verrou = portMUX_INITIALIZER_UNLOCKED;
static TaskHandle_t s_tache = nullptr;

static void reveiller() {
  if (s_tache) xTaskNotifyGive(s_tache);
}

// ---------- événements de la pile Bluetooth (tâche Bluetooth : rester bref) ----------

static void evenement(esp_hf_client_cb_event_t ev, esp_hf_client_cb_param_t *p) {
  switch (ev) {
    case ESP_HF_CLIENT_CONNECTION_STATE_EVT:
      if (p->conn_stat.state == ESP_HF_CLIENT_CONNECTION_STATE_SLC_CONNECTED) {
        s_slc = true;
      } else if (p->conn_stat.state == ESP_HF_CLIENT_CONNECTION_STATE_DISCONNECTED) {
        s_slc = false;
        s_appel = s_preparation = 0;
      }
      break;
    case ESP_HF_CLIENT_AUDIO_STATE_EVT:
      if (p->audio_stat.state == ESP_HF_CLIENT_AUDIO_STATE_CONNECTED ||
          p->audio_stat.state == ESP_HF_CLIENT_AUDIO_STATE_CONNECTED_MSBC) {
        esp_hf_client_register_data_callback(voixRecu, voixAEnvoyer);
        uint32_t f = p->audio_stat.state == ESP_HF_CLIENT_AUDIO_STATE_CONNECTED_MSBC ? 16000 : 8000;
        voixDemarrer(f);
        s_audio = f;
      } else if (p->audio_stat.state == ESP_HF_CLIENT_AUDIO_STATE_DISCONNECTED) {
        voixArreter();
        s_audio = 0;
      }
      break;
    case ESP_HF_CLIENT_CIND_CALL_EVT:
      s_appel = p->call.status == ESP_HF_CALL_STATUS_CALL_IN_PROGRESS;
      break;
    case ESP_HF_CLIENT_CIND_CALL_SETUP_EVT:
      s_preparation = (uint8_t)p->call_setup.status;
      if (s_preparation == ESP_HF_CALL_SETUP_STATUS_IDLE && !s_appel) {
        portENTER_CRITICAL(&s_verrou);
        s_numero[0] = 0;
        portEXIT_CRITICAL(&s_verrou);
      }
      break;
    case ESP_HF_CLIENT_RING_IND_EVT:
      // Certains téléphones envoient « RING » avant l'indicateur d'appel entrant
      if (!s_appel && s_preparation == ESP_HF_CALL_SETUP_STATUS_IDLE)
        s_preparation = ESP_HF_CALL_SETUP_STATUS_INCOMING;
      break;
    case ESP_HF_CLIENT_CLIP_EVT:
      if (p->clip.number) {
        portENTER_CRITICAL(&s_verrou);
        strlcpy(s_numero, p->clip.number, sizeof s_numero);
        portEXIT_CRITICAL(&s_verrou);
      }
      break;
    case ESP_HF_CLIENT_VOLUME_CONTROL_EVT:
      if (p->volume_control.type == ESP_HF_VOLUME_CONTROL_TARGET_SPK) {
        s_volume = (uint8_t)constrain(p->volume_control.volume, 0, 15);
        voixSetVolume(s_volume);
      }
      break;
    default:
      return;
  }
  reveiller();
}

// ---------- tâche « appel » : sonnerie, source de l'ampli, états ----------

static EtatAppel calculerEtat() {
  if (!s_slc) return EtatAppel::AUCUN;
  if (s_appel) return EtatAppel::EN_COURS;
  if (s_preparation == ESP_HF_CALL_SETUP_STATUS_INCOMING) return EtatAppel::SONNERIE;
  if (s_preparation == ESP_HF_CALL_SETUP_STATUS_OUTGOING_DIALING ||
      s_preparation == ESP_HF_CALL_SETUP_STATUS_OUTGOING_ALERTING)
    return EtatAppel::SORTANT;
  return EtatAppel::AUCUN;
}

static void tacheAppel(void *) {
  uint32_t telSansMainsLibres = 0;   // depuis quand le téléphone est connecté sans HFP
  uint8_t tentatives = 0;
  for (;;) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(1000));

    EtatAppel e = calculerEtat();
    if (e != s_etat) {
      s_etat = e;
      Serial.printf("[Appel] %s\n", appelEtatTexte());
    }

    // Une seule source pour l'ampli : la sonnerie tant que l'appel entrant n'est ni décroché
    // ni rejeté, sinon la voix d'appel, sinon la musique.
    if (e == EtatAppel::SONNERIE && sonnerieDisponible()) {
      audioSetMode(ModeAudio::SONNERIE, sonnerieFrequence());
      sonnerieDemarrer();
    } else {
      sonnerieArreter();
      if (s_audio) audioSetMode(ModeAudio::APPEL, s_audio);
      else audioSetMode(ModeAudio::MUSIQUE);
    }

    EventBits_t a = 0;
    if (s_slc) a |= SYS_HFP_CONNECTE;
    if (e == EtatAppel::SONNERIE) a |= SYS_SONNERIE;
    if (e == EtatAppel::EN_COURS || e == EtatAppel::SORTANT) a |= SYS_APPEL;
    sysClear((SYS_HFP_CONNECTE | SYS_SONNERIE | SYS_APPEL) & ~a);
    sysSet(a);

    // Le téléphone a connecté la musique mais pas les appels : on le lui demande
    // (3 essais, par exemple si le téléphone a été appairé avant la version 2.0.0).
    if (sysIs(SYS_TEL_CONNECTE) && !s_slc) {
      if (!telSansMainsLibres) telSansMainsLibres = millis();
      uint8_t adr[6];
      if (tentatives < 3 && millis() - telSansMainsLibres > 6000UL * (tentatives + 1) && musiqueAdresseTel(adr)) {
        tentatives++;
        Serial.printf("[Appel] Connexion mains libres au téléphone (essai %u)\n", tentatives);
        esp_hf_client_connect(adr);
      }
    } else {
      telSansMainsLibres = 0;
      if (!sysIs(SYS_TEL_CONNECTE)) tentatives = 0;
    }
  }
}

// ---------- API ----------

void appelDemarrer() {
  sonnerieInit();
  voixInit();
  esp_hf_client_register_callback(evenement);
  if (esp_hf_client_init() != ESP_OK) {
    Serial.println("[Appel] Impossible de démarrer le profil mains libres (HFP)");
    return;
  }
  xTaskCreate(tacheAppel, "appel", APPEL_PILE, nullptr, APPEL_PRIO, &s_tache);
  Serial.println("[Appel] Profil mains libres prêt");
}

EtatAppel appelEtat() { return s_etat; }

const char *appelEtatTexte() {
  switch (s_etat) {
    case EtatAppel::SONNERIE: return "appel entrant";
    case EtatAppel::SORTANT:  return "appel sortant";
    case EtatAppel::EN_COURS: return "appel en cours";
    default:                  return "pas d'appel";
  }
}

void appelNumero(char *dst, size_t taille) {
  portENTER_CRITICAL(&s_verrou);
  strlcpy(dst, s_numero, taille);
  portEXIT_CRITICAL(&s_verrou);
}

void appelDecrocher() {
  if (s_etat != EtatAppel::SONNERIE) return;
  Serial.println("[Appel] Décroché");
  sonnerieArreter();
  esp_hf_client_answer_call();
}

void appelRejeter() {
  if (s_etat != EtatAppel::SONNERIE) return;
  Serial.println("[Appel] Rejeté");
  sonnerieArreter();
  esp_hf_client_reject_call();   // AT+CHUP
}

void appelRaccrocher() {
  if (s_etat != EtatAppel::EN_COURS && s_etat != EtatAppel::SORTANT) return;
  Serial.println("[Appel] Raccroché");
  esp_hf_client_reject_call();   // AT+CHUP met aussi fin à l'appel en cours
}

void appelVolumePlus(int pas) {
  s_volume = (uint8_t)constrain((int)s_volume + pas, 0, 15);
  voixSetVolume(s_volume);
  if (s_slc) esp_hf_client_volume_update(ESP_HF_VOLUME_CONTROL_TARGET_SPK, s_volume);
  Serial.printf("[Appel] Volume : %u / 15\n", s_volume);
}

uint8_t appelVolume() { return s_volume; }
