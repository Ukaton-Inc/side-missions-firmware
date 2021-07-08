#pragma once
#ifndef _BLE_
#define _BLE_

#include <Arduino.h>

#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#define DEFAULT_BLE_NAME "Ukaton Side Mission"
#define MAX_CHARACTERISTIC_VALUE_LENGTH 23
#define GENERATE_UUID(val) ("5691eddf-" val "-4420-b7a5-bb8751ab5181")

namespace ble
{    
    extern BLEServer *pServer;
    class ServerCallbacks;
    extern BLEService *pService;

    extern BLEAdvertising *pAdvertising;
    extern BLEAdvertisementData *pAdvertisementData;

    extern bool isServerConnected;

    void setup();

    BLECharacteristic *createCharacteristic(const char *uuid, uint32_t properties, const char *name, BLEService *_pService = pService);

    void start();
} // namespace ble

#endif // _BLE_