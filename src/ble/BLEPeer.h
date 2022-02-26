#pragma once
#ifndef _BLE_PEER_
#define _BLE_PEER_

#include "definitions.h"
#include "ble.h"
#include "information/type.h"
#include "sensor/sensorData.h"
#include <Preferences.h>

#include "NimBLEDevice.h"

#define BLE_PEER_UUID_PREFIX "90"

class BLEPeer
{
private:
    static BLEPeer peers[NIMBLE_MAX_CONNECTIONS];

public:
    static void setup();

private:
    void _setup(uint8_t index);
    Preferences preferences;

private:
    uint8_t index;
    std::string _name;
    type::Type _type;
    bool autoConnect = false;
    bool isConnected = false;
    sensorData::Configurations configurations;

private:
    BLECharacteristic *pNameCharacteristic = nullptr;
    BLECharacteristic *pConnectCharacteristic = nullptr;
    BLECharacteristic *pIsConnectedCharacteristic = nullptr;
    BLECharacteristic *pTypeCharacteristic = nullptr;
    BLECharacteristic *pSensorConfigurationCharacteristic = nullptr;
    BLECharacteristic *pSensorDataCharacteristic = nullptr;
    void formatBLECharacteristicUUID(char *buffer, uint8_t value);
    void formatBLECharacteristicName(char *buffer, const char *name);

private:
    class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
    {
    public:
        void onResult(BLEAdvertisedDevice *advertisedDevice);
    };

private:
    class Callbacks {
        private:
        BLEPeer *peer = nullptr;

        public:
        Callbacks(BLEPeer *_peer)
        {
            peer = _peer;
        }
    };
    class CharacteristicCallbacks : public Callbacks, public BLECharacteristicCallbacks
    {
        using Callbacks::Callbacks;
    };
    class ClientCallbacks : public Callbacks, public BLEClientCallbacks
    {
        using Callbacks::Callbacks;
    };

private:
    static unsigned long currentTime;

public:
    static void loop();

private:
    void _loop();

public:
    ~BLEPeer();
}; // class BLEPeer

#endif // _BLE_PEER_