#pragma once
#ifndef _WIFI_SERVER_
#define _WIFI_SERVER_

#include "definitions.h"
#include <Arduino.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

namespace wifiServer
{
    bool areMacAddressesEqual(const uint8_t *a, const uint8_t *b);

    enum class DeviceType : uint8_t
    {
        MOTION_MODULE,
        LEFT_INSOLE,
        RIGHT_INSOLE
    };

    bool isConnectedToClient();

    enum class MessageType : uint8_t
    {
        TIMESTAMP,

        GET_NUMBER_OF_DEVICES,
        AVAILABILITY,

        DEVICE_ADDED,
        DEVICE_REMOVED,

        BATTERY_LEVEL,

        GET_NAME,
        SET_NAME,

        GET_TYPE,

        MOTION_CALIBRATION,
        
        GET_MOTION_CONFIGURATION,
        SET_MOTION_CONFIGURATION,

        GET_PRESSURE_CONFIGURATION,
        SET_PRESSURE_CONFIGURATION,

        MOTION_DATA,
        PRESSURE_DATA,

        PING,
        
        CLIENT_CONNECTED,
        CLIENT_DISCONNECTED
    };

    enum class ErrorMessageType : uint8_t
    {
        NO_ERROR,
        DEVICE_NOT_FOUND,
        FAILED_TO_SEND,
    };

    extern std::map<uint8_t, std::map<MessageType, std::vector<uint8_t>>> deviceClientMessageMaps;
    extern std::map<MessageType, std::vector<uint8_t>> clientMessageMap;
    extern bool shouldSendToClient;
    extern unsigned long lastTimeConnected;

    extern unsigned long currentMillis;
    extern unsigned long previousDataMillis;

    const unsigned long dataInterval = 20;
    extern bool includeTimestampInClientMessage;

    void setup();
    void loop();
} // namespace wifiServer

#endif // _WIFI_SERVER_