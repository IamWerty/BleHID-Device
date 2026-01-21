/**
 * ESP32 Airmouse - BLE Handler
 */

#ifndef BLE_HANDLER_H
#define BLE_HANDLER_H

#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include "config.h"
#include "hidDescriptor.h"

// ============================================================================
// EXTERNAL VARIABLES
// ============================================================================
extern bool connected;
extern NimBLEHIDDevice* hid;
extern NimBLECharacteristic* inputMouse;

// ============================================================================
// BLE CALLBACKS
// ============================================================================

class BLECallbacks : public NimBLEServerCallbacks {
  void onConnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo) {
    connected = true;
    digitalWrite(LED_PIN, HIGH);
    Serial.println("BLE з'єднано");
  }
  
  void onDisconnect(NimBLEServer* pServer, NimBLEConnInfo& connInfo, int reason) {
    connected = false;
    digitalWrite(LED_PIN, LOW);
    Serial.println("BLE роз'єднано");
    pServer->startAdvertising();
  }
};

// ============================================================================
// INITIALIZATION
// ============================================================================

void initBLE() {
  Serial.println("Ініціалізація BLE...");
  
  NimBLEDevice::init("Airmouse");
  NimBLEDevice::setSecurityAuth(true, true, true);
  
  NimBLEServer* pServer = NimBLEDevice::createServer();
  pServer->setCallbacks(new BLECallbacks());
  
  hid = new NimBLEHIDDevice(pServer);
  inputMouse = hid->getInputReport(0);
  hid->setManufacturer("protomors");
  hid->setPnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->setHidInfo(0x00, 0x02);
  hid->setReportMap((uint8_t*)_hidReportDescriptor, sizeof(_hidReportDescriptor));
  hid->startServices();
  
  NimBLEAdvertising* pAdvertising = pServer->getAdvertising();
  pAdvertising->setAppearance(HID_MOUSE);
  pAdvertising->addServiceUUID(hid->getHidService()->getUUID());
  
  NimBLEAdvertisementData scanRespData;
  scanRespData.setName("Airmouse");
  pAdvertising->setScanResponseData(scanRespData);
  
  pAdvertising->setMinInterval(0x20);
  pAdvertising->setMaxInterval(0x40);
  pAdvertising->start();
  
  hid->setBatteryLevel(80);
  
  Serial.println("BLE готово!");
}

#endif