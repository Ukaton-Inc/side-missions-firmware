#pragma once
#ifndef _WIFI_SERVER_
#define _WIFI_SERVER_

#include <ESPAsyncWebServer.h>

namespace wifiServer
{
    extern AsyncWebServer server;

    void setup();
    void loop();
} // namespace wifiServer

#endif // _WIFI_SERVER_