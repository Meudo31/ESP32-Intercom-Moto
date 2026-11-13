#include "musique_bt.h"

#include <Arduino.h>
#include "BluetoothA2DPSink.h"
#include "audio.h"
#include "etat.h"
#include "system_events.h"

static BluetoothA2DPSink a2dp;

// ---------- callbacks de la pile Bluetooth ----------

static void copier(char *dst, size_t taille, const uint8_t *src) {
  strlcpy(dst, src ? (const char *)src : "", taille);
}

static void connexionChangee(esp_a2d_connection_state_t etat, void *) {
  bool connecte = etat == ESP_A2D_CONNECTION_STATE_CONNECTED;
  if (connecte) {
    sysSet(SYS_TEL_CONNECTE);
  } else {
    sysClear(SYS_TEL_CONNECTE | SYS_LECTURE);
  }
  EtatVerrou v;
  if (connecte) {
    const uint8_t *a = *a2dp.get_current_peer_address();
    snprintf(g_etat.telAdresse, sizeof g_etat.telAdresse, "%02X:%02X:%02X:%02X:%02X:%02X",
             a[0], a[1], a[2], a[3], a[4], a[5]);
    strlcpy(g_etat.telNom, a2dp.get_peer_name(), sizeof g_etat.telNom);
  } else {
    g_etat.telNom[0] = 0;
    g_etat.telAdresse[0] = 0;
    g_etat.titre[0] = g_etat.artiste[0] = g_etat.album[0] = g_etat.genre[0] = 0;
    g_etat.piste = g_etat.nbPistes = 0;
    g_etat.dureeMs = g_etat.positionMs = 0;
    g_etat.avrcp = false;
    g_etat.rssi = 0;
  }
  g_etat.compteur++;
  Serial.println(connecte ? "[Musique] Téléphone connecté" : "[Musique] Téléphone déconnecté");
}

static void audioChange(esp_a2d_audio_state_t etat, void *) {
  if (etat == ESP_A2D_AUDIO_STATE_STARTED) sysSet(SYS_LECTURE);
  else sysClear(SYS_LECTURE);
}

static void avrcpChange(bool connecte) {
  EtatVerrou v;
  g_etat.avrcp = connecte;
  // Le nom du téléphone est souvent connu un peu après la connexion
  if (sysIs(SYS_TEL_CONNECTE)) strlcpy(g_etat.telNom, a2dp.get_peer_name(), sizeof g_etat.telNom);
  g_etat.compteur++;
}

static void metadonnees(uint8_t id, const uint8_t *texte) {
  EtatVerrou v;
  switch (id) {
    case ESP_AVRC_MD_ATTR_TITLE:        copier(g_etat.titre, sizeof g_etat.titre, texte); break;
    case ESP_AVRC_MD_ATTR_ARTIST:       copier(g_etat.artiste, sizeof g_etat.artiste, texte); break;
    case ESP_AVRC_MD_ATTR_ALBUM:        copier(g_etat.album, sizeof g_etat.album, texte); break;
    case ESP_AVRC_MD_ATTR_GENRE:        copier(g_etat.genre, sizeof g_etat.genre, texte); break;
    case ESP_AVRC_MD_ATTR_TRACK_NUM:    g_etat.piste = atoi((const char *)texte); break;
    case ESP_AVRC_MD_ATTR_NUM_TRACKS:   g_etat.nbPistes = atoi((const char *)texte); break;
    case ESP_AVRC_MD_ATTR_PLAYING_TIME: g_etat.dureeMs = strtoul((const char *)texte, nullptr, 10); break;
    default: return;
  }
  g_etat.compteur++;
}

static void statutLecture(esp_avrc_playback_stat_t statut) {
  if (statut == ESP_AVRC_PLAYBACK_PLAYING) sysSet(SYS_LECTURE);
  else if (statut == ESP_AVRC_PLAYBACK_PAUSED || statut == ESP_AVRC_PLAYBACK_STOPPED) sysClear(SYS_LECTURE);
}

static void position(uint32_t ms) {
  EtatVerrou v;
  g_etat.positionMs = ms;
}

static void frequence(uint16_t f) {
  audioFrequence(f);
  EtatVerrou v;
  g_etat.frequence = f;
}

// ---------- API ----------

void musiqueDemarrer(const char *nom) {
  a2dp.set_default_bt_mode(ESP_BT_MODE_BTDM);   // garde le BLE pour l'app Android
  a2dp.set_stream_reader(audioRecevoir, false);  // false : on gère l'I2S nous-mêmes
  a2dp.set_sample_rate_callback(frequence);
  a2dp.set_on_connection_state_changed(connexionChangee);
  a2dp.set_on_audio_state_changed(audioChange);
  a2dp.set_avrc_connection_state_callback(avrcpChange);
  a2dp.set_avrc_metadata_attribute_mask(ESP_AVRC_MD_ATTR_TITLE | ESP_AVRC_MD_ATTR_ARTIST |
                                        ESP_AVRC_MD_ATTR_ALBUM | ESP_AVRC_MD_ATTR_TRACK_NUM |
                                        ESP_AVRC_MD_ATTR_NUM_TRACKS | ESP_AVRC_MD_ATTR_GENRE |
                                        ESP_AVRC_MD_ATTR_PLAYING_TIME);
  a2dp.set_avrc_metadata_callback(metadonnees);
  a2dp.set_avrc_rn_playstatus_callback(statutLecture);
  a2dp.set_avrc_rn_play_pos_callback(position, 1);
  a2dp.set_rssi_active(true);
  a2dp.start(nom);  // reconnexion automatique au dernier téléphone
  Serial.printf("[Musique] Prêt : cherche « %s » dans le Bluetooth du téléphone\n", nom);
}

void musiqueArreter() { a2dp.end(); }

void musiqueLecture() { a2dp.play(); }
void musiquePause() { a2dp.pause(); }
void musiqueBasculer() {
  if (sysIs(SYS_LECTURE)) a2dp.pause();
  else a2dp.play();
}
void musiqueStop() { a2dp.stop(); }
void musiqueSuivant() { a2dp.next(); }
void musiquePrecedent() { a2dp.previous(); }
void musiqueSetVolumeTel(uint8_t v) { a2dp.set_volume(v > 127 ? 127 : v); }
uint8_t musiqueVolumeTel() { return (uint8_t)a2dp.get_volume(); }
void musiqueDeconnecter() { a2dp.disconnect(); }
void musiqueReconnecter() { a2dp.reconnect(); }
void musiqueOublier() {
  a2dp.disconnect();
  a2dp.clean_last_connection();
}

bool musiqueAdresseTel(uint8_t adresse[6]) {
  if (!sysIs(SYS_TEL_CONNECTE)) return false;
  memcpy(adresse, *a2dp.get_current_peer_address(), 6);
  return true;
}

void musiqueRafraichirSignal() {
  if (!sysIs(SYS_TEL_CONNECTE)) return;
  a2dp.update_rssi();   // la réponse arrive plus tard : on lit la mesure précédente
  int8_t r = a2dp.get_last_rssi().rssi_delta;
  EtatVerrou v;
  g_etat.rssi = r;
}
