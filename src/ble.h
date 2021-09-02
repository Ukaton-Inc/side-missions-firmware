#pragma once
#ifndef _BLE_
#define _BLE_

#include <Arduino.h>

/*
#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
*/

#include <NimBLEDevice.h>

#define DEFAULT_BLE_NAME "Ukaton Side Mission"
#define GENERATE_UUID(val) ("5691eddf-" val "-4420-b7a5-bb8751ab5181")

namespace ble
{
    constexpr uint8_t max_characteristic_value_length = 23;

    extern BLEServer *pServer;
    extern unsigned long lastTimeConnected;
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