#pragma once
#ifndef _ESP_NOW_PEER_
#define _ESP_NOW_PEER_

#include "definitions.h"

#include <Arduino.h>

#include <esp_now.h>
#include <esp_wifi.h>

#include "wifiServer.h"

class EspNowPeer
{
public:
    static std::vector<EspNowPeer *> peers;
    EspNowPeer(const uint8_t *macAddress);
    ~EspNowPeer();

private:
    uint8_t index;
    uint8_t deviceIndex;

public:
    uint8_t getIndex();
    uint8_t getDeviceIndex();
    void setIndex(uint8_t index);
    static EspNowPeer *getPeerByDeviceIndex(uint8_t deviceIndex);

private:
    uint8_t macAddress[MAC_ADDRESS_SIZE];

public:
    const uint8_t *getMacAddress();
    void setMacAddress(const uint8_t *macAddress);
    static EspNowPeer *getPeerByMacAddress(const uint8_t *macAddress);

public:
    esp_err_t connect();
    esp_err_t disconnect();
    bool isConnected();

private:
    esp_now_peer_info_t peerInfo;

private:
    bool isAvailable = true;

public:
    bool getAvailability();
    void updateAvailability(bool isAvailable);

private:
    uint8_t batteryLevel = 0;

public:
    uint8_t getBatteryLevel();
    void updateBatteryLevel(uint8_t batteryLevel);
    bool didSendBatteryLevel = false;

private:
    std::string name;

public:
    bool didUpdateNameAtLeastOnce = false;
    const std::string *getName();
    void updateName(const char *name, size_t length);

private:
    wifiServer::DeviceType deviceType;

public:
    bool didUpdateDeviceTypeAtLeastOnce = false;
    wifiServer::DeviceType getDeviceType();
    void updateDeviceType(wifiServer::DeviceType deviceType);

private:
    class Motion
    {
    public:
        ~Motion();

    private:
        uint16_t configuration[6]{0}; // FIX - replace with motion::NUMBER_OF_DATA_TYPES
        uint8_t calibration[4]{0};    // FIX - replace with motion::NUMBER_OF_CALIBRATION_TYPES
        uint8_t *data = nullptr;
        uint8_t dataSize = 0;
        uint8_t *dataTypes = nullptr;
    };
    Motion *motion = nullptr;

    class Pressure
    {
    public:
        ~Pressure();

    private:
        uint16_t configuration[6]{0}; // FIX - replace with pressure::NUMBER_OF_DATA_TYPES
        uint8_t *data = nullptr;
        uint8_t dataSize = 0;
        uint8_t *dataTypes = nullptr;
    };
    Pressure *pressure = nullptr;

private:
    bool shouldSend;

public:
    std::map<wifiServer::MessageType, std::vector<uint8_t>> messageMap;
    void send();
    static bool shouldSendAll;
    static void sendAll();

private:
    static unsigned long previousPingMillis;
    static const long pingInterval = 2000;

public:
    void ping();
    static void pingAll();

public:
    void onMessage(const uint8_t *incomingData, int len);

private:
    void batteryLevelLoop();

public:
    static void pingLoop();
    static void batteryLevelsLoop();
};

#endif // _ESP_NOW_PEER_