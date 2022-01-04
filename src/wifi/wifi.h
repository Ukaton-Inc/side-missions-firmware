#pragma once
#ifndef _WIFI_
#define _WIFI_

#include <Arduino.h>
#include <WiFi.h>

namespace wifi
{
    const std::string *getSSID();
    const std::string *getPassword();
    
    void setSSID(const char* ssid, bool commit = true);
    void setPassword(const char* password, bool commit = true);

    bool getAutoConnect();
    void setAutoConnect(bool autoConnect, bool commit = true);

    void setup();
    void loop();

    void connect();
    void disconnect();
} // namespace wifi

#endif // _WIFI_