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

class BLEPeer_ : public BLECharacteristicCallbacks, public BLEClientCallbacks
{
private:
    static unsigned long currentTime;
private:
    static BLEScan* pBLEScan;
    static bool shouldScan;
    static void updateShouldScan();
    static constexpr uint16_t check_scan_interval_ms = 1000;
    static unsigned long lastScanCheck;
    static void checkScan();
    class AdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
        public:
        void onResult(BLEAdvertisedDevice* advertisedDevice);
    };

private:
    static BLEPeer_ peers[NIMBLE_MAX_CONNECTIONS];

private:
    uint8_t index;
    Preferences preferences;

private:
    std::string _name;
    bool autoConnect = false;
    NimBLEAdvertisedDevice* pAdvertisedDevice = nullptr;
    bool foundDevice = false;
    NimBLEClient* pClient = nullptr;
    bool connectToDevice();
    bool isConnected();

private:
    NimBLERemoteService* pRemoteService = nullptr;
    NimBLERemoteCharacteristic* pRemoteNameCharacteristic = nullptr;
    NimBLERemoteCharacteristic* pRemoteTypeCharacteristic = nullptr;
    NimBLERemoteCharacteristic* pRemoteSensorConfigurationCharacteristic = nullptr;
    NimBLERemoteCharacteristic* pRemoteSensorDataCharacteristic = nullptr;
    static void onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    void _onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

private:
    type::Type _type = type::Type::MOTION_MODULE;
    bool isValidSensorType(sensorData::SensorType);
    sensorData::Configurations configurations;
    uint8_t sensorData[2 + sizeof(sensorData::motionData) + 2 + sizeof(sensorData::pressureData)];
    uint8_t sensorDataSize = 0;

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

    bool shouldChangeName = false;
    void onNameWrite();
    void changeName();

    bool shouldDisconnect = false;
    void onConnectWrite();

    bool shouldChangeType = false;
    type::Type typeToChangeTo;
    void onTypeWrite();
    void changeType();

    bool shouldChangeSensorConfiguration = false;
    std::string receivedConfiguration;
    void onSensorConfigurationWrite();
    void changeSensorConfiguration();

    bool shouldNotifySensorData = false;

    void notifySensorData();

public:
    void onWrite(BLECharacteristic *pCharacteristic);
    void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue);

public:
    void onConnect(NimBLEClient* pClient);
    void onDisconnect(NimBLEClient* pClient);
    bool shouldUpdateIsConnected = false;
    void updateIsConnectedCharacteristic(bool notify = true);
    bool onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params);

public:
    static bool isServerConnected;
    void disconnect();
    static void onServerConnect();
    static void onServerDisconnect();

public:
    static void setup();

private:
    void _setup(uint8_t index);

public:
    static void loop();

private:
    void _loop();

public:
    ~BLEPeer_();
}; // class BLEPeer_

#endif // _BLE_PEER_