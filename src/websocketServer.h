#pragma once
#ifndef _WEBSOCKET_SERVER_
#define _WEBSOCKET_SERVER_

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

namespace websocketServer
{
    extern AsyncWebServer server;
    extern AsyncWebSocket ws;
    void setup();
    void loop();
} // namespace websocketServer

#endif // _WEBSOCKET_SERVER_