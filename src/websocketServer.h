#pragma once
#ifndef _WEBSOCKET_SERVER_
#define _WEBSOCKET_SERVER_

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

namespace websocketServer
{
    typedef enum: uint8_t
    {
        GET_NAME_REQUEST = 0,
        GET_NAME_RESPONSE,

        SET_NAME_REQUEST,
        SET_NAME_RESPONSE,

        GET_IMU_CONFIGURATION_REQUEST,
        GET_IMU_CONFIGURATION_RESPONSE,

        SET_IMU_CONFIGURATION_REQUEST,
        SET_IMU_CONFIGURATION_RESPONSE,

        IMU_DATA,

        GET_IMU_CALLIBRATION_DATA_REQUEST,
        GET_IMU_CALLIBRATION_DATA_RESPONSE,

        GET_BATTERY_LEVEL_REQUEST,
        GET_BATTERY_LEVEL_RESPONSE,

        GET_PRESSURE_CONFIGURATION_REQUEST,
        GET_PRESSURE_CONFIGURATION_RESPONSE,

        SET_PRESSURE_CONFIGURATION_REQUEST,
        SET_PRESSURE_CONFIGURATION_RESPONSE,

        PRESSURE_DATA
    } MessageType;

    extern unsigned long lastTimeConnected;
    extern AsyncWebServer server;
    extern AsyncWebSocket ws;
    void setup();
    void loop();
} // namespace websocketServer

#endif // _WEBSOCKET_SERVER_