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
        GET_NUMBER_OF_DEVICES,

        DEVICE_ADDED,
        DEVICE_REMOVED,

        GET_AVAILABILITY,

        BATTERY_LEVEL,

        GET_NAME,
        SET_NAME,

        GET_TYPE,

        GET_MOTION_CONFIGURATION,
        SET_MOTION_CONFIGURATION,

        GET_MOTION_CALLIBRATION_DATA,

        GET_PRESSURE_CONFIGURATION,
        SET_PRESSURE_CONFIGURATION,

        DATA,

        PING
    };

    enum class ErrorMessageType : uint8_t
    {
        NO_ERROR,
        DEVICE_NOT_FOUND,
        FAILED_TO_SEND,
    };

    enum class DataType : uint8_t
    {
        
    };

    extern std::map<uint8_t, std::map<MessageType, std::vector<uint8_t>>> deviceClientMessageMaps;
    extern std::map<MessageType, std::vector<uint8_t>> clientMessageMap;
    extern bool shouldSendToClient;

    extern unsigned long currentMillis;

    void setup();
    void loop();
} // namespace wifiServer

#endif // _WIFI_SERVER_