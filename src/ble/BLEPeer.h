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
    static constexpr uint16_t check_server_connection_interval_ms = 1000;
    static unsigned long lastServerConnectionCheck;
    static bool isServerConnected;
    static void checkServerConnection();
    static void onServerConnectionUpdate();
    static void onServerConnect();
    static void onServerDisconnect();

private:
    static BLEScan* pBLEScan;
    static bool shouldScan;
    static void updateShouldScan();
    static constexpr uint16_t check_scan_interval_ms = 1000;
    static unsigned long lastScanCheck;
    static void checkScan();

private:
    uint8_t index;
    std::string _name;
    type::Type _type;
    bool autoConnect = false;
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
    bool shouldChangeNameCharacteristic = false;
    bool shouldChangeConnectionCharacteristic = false;
    bool shouldChangeTypeCharacteristic = false;
    bool shouldChangeSensorConfigurationCharacteristic = false;
    bool shouldNotifySensorData = false;

    void changeNameCharacteristic();
    void changeConnectionCharacteristic(bool notify = true);
    void changeTypeCharacteristic();
    void changeSensorConfigurationCharacteristic();
    void notifySensorDataCharacteristic();

private:
    class AdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
    {
    public:
        void onResult(BLEAdvertisedDevice *advertisedDevice) {
            BLEPeer::onAdvertisedDevice(advertisedDevice);
        }
    };
    static void onAdvertisedDevice(BLEAdvertisedDevice *advertisedDevice);

private:
    class Callbacks {
        public:
        BLEPeer *peer = nullptr;
        Callbacks(BLEPeer *_peer)
        {
            peer = _peer;
        }
        ~Callbacks() {
            peer = nullptr;
        }
    };
    
    class ClientCallbacks : public Callbacks, public BLEClientCallbacks
    {
        public:
        using Callbacks::Callbacks;

        void onConnect(NimBLEClient* pClient) {
            peer->onClientConnect(pClient);
        }
        void onDisconnect(NimBLEClient* pClient) {
            peer->onClientDisconnect(pClient);
        }
        bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
            return peer->onClientConnectionParamsUpdateRequest(pClient, params);
        }
    };
    void onClientConnect(NimBLEClient* pClient);
    void onClientDisconnect(NimBLEClient* pClient);
    void onClientConnectionUpdate();
    bool onClientConnectionParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params);

private:
    NimBLERemoteService* pRemoteService = nullptr;
    NimBLERemoteCharacteristic* pRemoteNameCharacteristic = nullptr;
    NimBLERemoteCharacteristic* pRemoteTypeCharacteristic = nullptr;
    NimBLERemoteCharacteristic* pRemoteSensorConfigurationCharacteristic = nullptr;
    NimBLERemoteCharacteristic* pRemoteSensorDataCharacteristic = nullptr;

    static void onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
    void _onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);

private:
    class CharacteristicCallbacks : public Callbacks, public BLECharacteristicCallbacks
    {
        public:
        using Callbacks::Callbacks;

        void onWrite(BLECharacteristic *pCharacteristic) {
            peer->onCharacteristicWrite(pCharacteristic);
        }
    };
    void onCharacteristicWrite(BLECharacteristic *pCharacteristic);

    void onNameCharacteristicWrite();
    void onConnectCharacteristicWrite();
    void onTypeCharacteristicWrite();
    void onSensorConfigurationCharacteristicWrite();
    std::string sensorConfigurationCharacteristicValue;
    std::string sensorDataCharacteristicValue;

    bool shouldChangeRemoteNameCharacteristic = false;
    bool shouldChangeConnection = false;
    bool shouldChangeRemoteTypeCharacteristic = false;
    bool shouldChangeRemoteSensorConfigurationCharacteristic = false;
    
    void changeRemoteNameCharacteristic();
    void changeConnection();
    void changeRemoteTypeCharacteristic();
    void changeRemoteSensorConfigurationCharacteristic();

private:
    NimBLEAdvertisedDevice* pAdvertisedDevice = nullptr;
    NimBLEClient* pClient = nullptr;

    bool isConnected = false;

    bool connectionUpdated = false;
    void onConnectionUpdate();
    void onConnection();
    void onDisconnection();

    bool shouldConnect = false;
    bool connect();

    bool shouldDisconnect = false;
    void disconnect();


private:
    static unsigned long currentTime;

public:
    static void loop();

private:
    void _loop();
}; // class BLEPeer

#endif // _BLE_PEER_