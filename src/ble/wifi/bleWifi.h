#pragma once
#ifndef _BLE_WIFI_
#define _BLE_WIFI_

#include "ble/ble.h"

namespace bleWifi
{
    extern BLECharacteristic *pSSIDCharacteristic;
    class SSIDCharacteristicCallbacks;

    extern BLECharacteristic *pPasswordCharacteristic;
    class PasswordCharacteristicCallbacks;    

    extern BLECharacteristic *pConnectCharacteristic;
    class ConnectCharacteristicCallbacks;

    extern BLECharacteristic *pIsConnectedCharacteristic;
    
    extern BLECharacteristic *pIPAddressCharacteristic;
    extern BLECharacteristic *pMACAddressCharacteristic;

    void setup();
    void loop();
} // namespace bleWifi

#endif // _BLE_WIFI_