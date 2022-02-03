#include "wifiServer.h"
#include "webSocket.h"
#include "udp.h"
#include "osc.h"

namespace wifiServer
{
    AsyncWebServer server(80);

    void setup()
    {
        webSocket::setup();
    }

    bool _isConnected = false;
    void _onConnectionUpdate()
    {
        if (_isConnected)
        {
            Serial.println("connected to the internet");
            server.begin();
            udp::listen();
            osc::listen();
        }
        else
        {
            Serial.println("not connected to the internet");
            server.end();
        }
    }

    constexpr uint16_t check_connection_delay_ms = 1000;
    unsigned long latestConnectionCheckTimestamp = 0;
    void _checkConnectionLoop()
    {
        auto isConnected = WiFi.isConnected();
        if (_isConnected != isConnected)
        {
            _isConnected = isConnected;
            _onConnectionUpdate();
        }
    }

    unsigned long currentTime = 0;
    void loop()
    {
        currentTime = millis();
        if (currentTime - latestConnectionCheckTimestamp > check_connection_delay_ms)
        {
            latestConnectionCheckTimestamp = currentTime - (currentTime % check_connection_delay_ms);
            _checkConnectionLoop();
        }
        
        if (_isConnected) {
            webSocket::loop();
            udp::loop();
            osc::loop();
        }
    }
} // namespace wifiServer
