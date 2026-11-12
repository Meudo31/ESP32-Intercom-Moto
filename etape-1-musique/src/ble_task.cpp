// Serveur BLE avec la pile Bluedroid d'Arduino : elle cohabite avec le Bluetooth classique
// de la musique (NimBLE ne le permet pas). À démarrer après musiqueDemarrer().
#include "ble_task.h"

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "app_data.h"
#include "ble_ota.h"
#include "ble_protocol.h"
#include "parametres.h"
#include "reglages.h"
#include "system_events.h"

static BLEServer *s_server = nullptr;
static BLECharacteristic *s_data = nullptr, *s_media = nullptr, *s_info = nullptr;
static BLECharacteristic *s_command = nullptr, *s_otaCtrl = nullptr, *s_otaData = nullptr;

// Les valeurs reçues sont std::string (Arduino 2) ou String (Arduino 3) : c_str() + length() marchent pour les deux.
template <typename T>
static std::string versStd(const T &v) { return std::string(v.c_str(), v.length()); }

static void repondreOta(const std::string &r) {
  s_otaCtrl->setValue((uint8_t *)r.data(), r.size());
  s_otaCtrl->notify();
}

// ---------------------------------------------------------------- callbacks

class ServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *) override {
    Serial.println("[BLE] App connectee");
    sysSet(SYS_BLE_CONNECTED);
  }
  void onDisconnect(BLEServer *srv) override {
    Serial.println("[BLE] App deconnectee");
    bleOtaAbort();
    sysClear(SYS_BLE_CONNECTED);
    BLEDevice::startAdvertising();
  }
};

class CommandCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    std::string v = versStd(c->getValue());
    appCommand(v.data(), v.size());
  }
};

class OtaCtrlCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override { repondreOta(bleOtaControl(versStd(c->getValue()))); }
};

class OtaDataCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *c) override {
    std::string v = versStd(c->getValue());
    std::string err = bleOtaData((const uint8_t *)v.data(), v.size());
    if (!err.empty()) repondreOta(err);
  }
};

class DataReadCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *c) override {
    uint8_t buf[APP_DATA_MAX];
    c->setValue(buf, appDataBuild(buf, sizeof buf));
  }
};

class MediaReadCallbacks : public BLECharacteristicCallbacks {
  void onRead(BLECharacteristic *c) override {
    std::string j = appMediaJson();
    c->setValue((uint8_t *)j.data(), j.size());
  }
};

// ---------------------------------------------------------------- mise en place

static BLECharacteristic *creer(BLEService *svc, const char *uuid, uint32_t props) {
  BLECharacteristic *c = svc->createCharacteristic(uuid, props);
  if (props & BLECharacteristic::PROPERTY_NOTIFY) c->addDescriptor(new BLE2902());
  return c;
}

static void bleSetup() {
  BLEDevice::init(reglagesNom());   // même nom que le Bluetooth musique
  BLEDevice::setMTU(517);

  s_server = BLEDevice::createServer();
  s_server->setCallbacks(new ServerCallbacks());

  // 7 caractéristiques + descripteurs : il faut réserver assez de « handles »
  BLEService *svc = s_server->createService(BLEUUID(BLE_UUID_SERVICE), 30);
  s_data = creer(svc, BLE_UUID_DATA, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  s_data->setCallbacks(new DataReadCallbacks());
  s_media = creer(svc, BLE_UUID_MEDIA, BLECharacteristic::PROPERTY_READ);
  s_media->setCallbacks(new MediaReadCallbacks());
  s_info = creer(svc, BLE_UUID_INFO, BLECharacteristic::PROPERTY_READ);
  std::string info = appInfoJson();
  s_info->setValue((uint8_t *)info.data(), info.size());
  s_command = creer(svc, BLE_UUID_COMMAND, BLECharacteristic::PROPERTY_WRITE);
  s_command->setCallbacks(new CommandCallbacks());
  s_otaCtrl = creer(svc, BLE_UUID_OTA_CTRL, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  s_otaCtrl->setCallbacks(new OtaCtrlCallbacks());
  s_otaData = creer(svc, BLE_UUID_OTA_DATA, BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_WRITE_NR);
  s_otaData->setCallbacks(new OtaDataCallbacks());
  svc->start();

  BLEAdvertising *adv = BLEDevice::getAdvertising();
  adv->addServiceUUID(BLE_UUID_SERVICE);
  adv->setScanResponse(true);
  BLEDevice::startAdvertising();
  Serial.printf("[BLE] App : visible sous le nom \"%s\"\n", reglagesNom());
}

// ---------------------------------------------------------------- tâche

static void bleTask(void *) {
  bleSetup();
  uint8_t buf[APP_DATA_MAX];

  for (;;) {
    if (bleOtaRestartPending()) {
      vTaskDelay(pdMS_TO_TICKS(800));   // laisse partir la notification "DONE"
      ESP.restart();
    }
    // Données vers le téléphone (suspendues pendant une mise à jour)
    if (sysIs(SYS_BLE_CONNECTED) && !bleOtaInProgress()) {
      size_t n = appDataBuild(buf, sizeof buf);
      if (n) {
        s_data->setValue(buf, n);
        s_data->notify();
      }
    }
    vTaskDelay(pdMS_TO_TICKS(BLE_NOTIFY_PERIOD_MS));
  }
}

void bleTaskStart() {
  xTaskCreate(bleTask, "ble", BLE_TASK_STACK, nullptr, BLE_TASK_PRIO, nullptr);
}
