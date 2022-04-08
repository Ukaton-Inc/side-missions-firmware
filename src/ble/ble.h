#pragma once
#ifndef _BLE_
#define _BLE_

#include <Arduino.h>
#include <NimBLEDevice.h>

#define UUID_PREFIX "5691eddf-"
#define UUID_SUFFIX "-4420-b7a5-bb8751ab5181"
#define GENERATE_UUID(val) (UUID_PREFIX val UUID_SUFFIX)

#define BLE_UUID_LENGTH 36
#define MAX_BLE_ATTRIBUTE_LENGTH 512

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
    void stop();
    void loop();
} // namespace ble

#endif // _BLE_