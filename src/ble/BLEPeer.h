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

class BLEPeer : public BLECharacteristicCallbacks, public BLEClientCallbacks
{
private:
    static unsigned long currentTime;
private:
    static BLEScan* pBLEScan;
    static bool shouldScan;
    static void updateShouldScan();
    static constexpr uint16_t check_scan_interval_ms = 2000;
    static unsigned long lastScanCheck;
    static void checkScan();
    class AdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
        public:
        void onResult(BLEAdvertisedDevice* advertisedDevice);
    };

private:
    static BLEPeer peers[NIMBLE_MAX_CONNECTIONS];

private:
    uint8_t index;
    Preferences preferences;

private:
    std::string name;
    bool autoConnect = false;
    NimBLEAdvertisedDevice* pAdvertisedDevice = nullptr;
    bool foundDevice = false;
    NimBLEClient* pClient = nullptr;
    bool connectToDevice();
    bool isConnected = false;

private:
    NimBLERemoteService* pRemoteService = nullptr;
    NimBLERemoteCharacteristic* pRemoteNameCharacteristic = nullptr;
    NimBLERemoteCharacteristic* pRemoteTypeCharacteristic = nullptr;
    NimBLERemoteCharacteristic* pRemoteSensorConfigurationCharacteristic = nullptr;
    NimBLERemoteCharacteristic* pRemoteSensorDataCharacteristic = nullptr;
    static void onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    static void _onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

private:
    type::Type type = type::Type::MOTION_MODULE;
    uint8_t sensorConfiguration[sizeof(sensorData::motionConfiguration) + sizeof(sensorData::pressureConfiguration)];
    uint8_t sensorData[2 + sizeof(sensorData::motionData) + 2 + sizeof(sensorData::pressureData)];

private:
    void formatBLECharacteristicUUID(char *buffer, uint8_t value);
    void formatBLECharacteristicName(char *buffer, const char *name);

private:
    BLECharacteristic *pNameCharacteristic = nullptr;
    BLECharacteristic *pConnectCharacteristic = nullptr;
    BLECharacteristic *pIsConnectedCharacteristic = nullptr;
    BLECharacteristic *pTypeCharacteristic = nullptr;
    BLECharacteristic *pSensorConfigurationCharacteristic = nullptr;
    BLECharacteristic *pSensorDataCharacteristic = nullptr;
    void onNameWrite();
    void onConnectWrite();
    void onTypeWrite();
    void onSensorConfigurationWrite();

public:
    void onWrite(BLECharacteristic *pCharacteristic);

public:
    void onConnect(NimBLEClient* pClient);
    void onDisconnect(NimBLEClient* pClient);

public:
    static void setup();

private:
    void _setup(uint8_t index);

public:
    static void loop();

private:
    void _loop();

public:
    ~BLEPeer();
}; // class BLEPeer

#endif // _BLE_PEER_