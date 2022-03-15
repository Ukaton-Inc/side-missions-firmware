#include "definitions.h"
#include <Preferences.h>
#include "wifi.h"
#include "wifiServer.h"

namespace wifi
{
    constexpr uint8_t max_ssid_size = 32;
    std::string ssid(WIFI_SSID);
    constexpr uint8_t max_password_size = 64;
    std::string password(WIFI_PASSWORD);

    bool _autoConnect = false;

    const std::string *getSSID()
    {
        return &ssid;
    }
    const std::string *getPassword()
    {
        return &password;
    }

    Preferences preferences;
    void setup()
    {
        preferences.begin("wifi");
        if (preferences.isKey("ssid"))
        {
            ssid = preferences.getString("ssid").c_str();
        }
        if (preferences.isKey("ssid"))
        {
            password = preferences.getString("password").c_str();
        }
        _autoConnect = preferences.getBool("autoConnect", _autoConnect);
        preferences.end();

        if (_autoConnect)
        {
            connect();
        }

        wifiServer::setup();
    }

    void setSSID(const char *_ssid, bool commit)
    {
        ssid.assign(_ssid, strlen(_ssid));

        preferences.begin("wifi");
        preferences.putString("ssid", ssid.c_str());
        preferences.end();

        Serial.print("new wifi ssid: ");
        Serial.println(ssid.c_str());
    }
    void setPassword(const char *_password, bool commit)
    {
        password.assign(_password, strlen(_password));

        preferences.begin("wifi");
        preferences.putString("password", password.c_str());
        preferences.end();

        Serial.print("new wifi password: ");
        Serial.println(password.c_str());
    }

    bool getAutoConnect()
    {
        return _autoConnect;
    }
    void setAutoConnect(bool autoConnect, bool commit)
    {
        if (_autoConnect != autoConnect)
        {
            _autoConnect = autoConnect;
            
            preferences.begin("wifi");
            preferences.putBool("autoConnect", _autoConnect);
            preferences.end();

            Serial.print("new wifi autoConnect: ");
            Serial.println(autoConnect);
        }
    }

    void connect()
    {
        WiFi.mode(WIFI_STA);
        WiFi.begin(ssid.c_str(), password.c_str());
        setAutoConnect(true);
    }
    void disconnect()
    {
        if (WiFi.isConnected())
        {
            Serial.println("disconnecting from WiFi...");
            WiFi.disconnect();
            Serial.println("disconnected from Wifi");
        }
        else
        {
            Serial.println("already disconnected");
        }

        WiFi.mode(WIFI_MODE_NULL);
        setAutoConnect(false);
    }

    void loop()
    {
        wifiServer::loop();
    }
} // namespace wifi