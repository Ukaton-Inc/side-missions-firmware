#pragma once
#ifndef _BLE_
#define _BLE_

#include <Arduino.h>
#include <NimBLEDevice.h>

#define GENERATE_UUID(val) ("5691eddf-" val "-4420-b7a5-bb8751ab5181")

namespace ble
{
    extern unsigned long lastTimeConnected;

    extern BLEServer *pServer;
    extern BLEService *pService;

    extern bool isServerConnected;
    class ServerCallbacks;

    extern BLEAdvertising *pAdvertising;
    extern BLEAdvertisementData *pAdvertisementData;

    void setup();

    BLECharacteristic *createCharacteristic(const char *uuid, uint32_t properties, const char *name, BLEService *_pService = pService);
    BLECharacteristic *createCharacteristic(BLEUUID uuid, uint32_t properties, const char *name, BLEService *_pService = pService);

    void start();
    void loop();
} // namespace ble

#endif // _BLE_