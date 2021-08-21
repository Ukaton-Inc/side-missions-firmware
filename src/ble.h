#pragma once
#ifndef _BLE_
#define _BLE_

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>

#define DEFAULT_BLE_NAME "Ukaton Motion Module"

namespace ble
{
    constexpr uint8_t max_characteristic_value_length = 23;

    extern BLEServer *pServer;
    class ServerCallbacks;
    extern BLEService *pService;

    extern bool isServerConnected;

    extern BLEAdvertising *pAdvertising;
    extern BLEAdvertisementData *pAdvertisementData;

    void setup();

    BLECharacteristic *createCharacteristic(const char *uuid, uint32_t properties, const char *name, BLEService *_pService = pService);
    BLECharacteristic *createCharacteristic(BLEUUID uuid, uint32_t properties, const char *name, BLEService *_pService = pService);

    void start();
} // namespace ble

#endif // _BLE_