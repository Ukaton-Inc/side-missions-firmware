#include "definitions.h"
#include "eepromUtils.h"
#include "wifi.h"
#include "wifiServer.h"

namespace wifi
{
    std::string ssid(WIFI_SSID);
    std::string password(WIFI_PASSWORD);

    constexpr uint8_t max_ssid_size = 32;
    constexpr uint8_t max_password_size = 64;

    bool _autoConnect = false;

    uint16_t ssidEepromAddress;
    uint16_t passwordEepromAddress;
    uint16_t autoConnectEepromAddress;

    void loadSSIDFromEEPROM()
    {
        ssid = EEPROM.readString(ssidEepromAddress).c_str();
        Serial.print("getting wifi ssid from EEPROM: ");
        Serial.println(ssid.c_str());
    }
    void loadPasswordFromEEPROM()
    {
        password = EEPROM.readString(passwordEepromAddress).c_str();
        Serial.print("getting wifi password from EEPROM: ");
        Serial.println(password.c_str());
    }
    void loadAutoConnectFromEEPROM()
    {
        _autoConnect = EEPROM.readBool(autoConnectEepromAddress);
        Serial.print("getting wifi autoconnect from EEPROM: ");
        Serial.println(_autoConnect);
    }
    void loadFromEEPROM()
    {
        loadSSIDFromEEPROM();
        loadPasswordFromEEPROM();
        loadAutoConnectFromEEPROM();
    }
    void saveSSIDToEEPROM(bool commit = true)
    {
        Serial.print("Saving wifi ssid to EEPROM: ");
        Serial.println(ssid.c_str());
        EEPROM.writeString(ssidEepromAddress, ssid.c_str());

        if (commit)
        {
            EEPROM.commit();
        }
    }
    void savePasswordToEEPROM(bool commit = true)
    {
        Serial.print("Saving wifi password to EEPROM: ");
        Serial.println(password.c_str());
        EEPROM.writeString(passwordEepromAddress, password.c_str());

        if (commit)
        {
            EEPROM.commit();
        }
    }

    void saveAutoConnectToEEPROM(bool commit = true)
    {
        Serial.print("Saving wifi autoconnect to EEPROM: ");
        Serial.println(_autoConnect);
        EEPROM.writeBool(autoConnectEepromAddress, _autoConnect);

        if (commit)
        {
            EEPROM.commit();
        }
    }
    void saveToEEPROM()
    {
        saveSSIDToEEPROM(false);
        savePasswordToEEPROM(false);
        saveAutoConnectToEEPROM(false);

        EEPROM.commit();
    }

    const std::string *getSSID()
    {
        return &ssid;
    }
    const std::string *getPassword()
    {
        return &password;
    }

    void setSSID(const char *_ssid, bool commit)
    {
        ssid.assign(_ssid, strlen(_ssid));
        saveSSIDToEEPROM(commit);
    }
    void setPassword(const char *_password, bool commit)
    {
        password.assign(_password, strlen(_password));
        savePasswordToEEPROM(commit);
    }

    bool getAutoConnect()
    {
        return _autoConnect;
    }
    void setAutoConnect(bool autoConnect, bool commit)
    {
        if (_autoConnect != autoConnect) {
            _autoConnect = autoConnect;
            saveAutoConnectToEEPROM(commit);
        }
    }

    void setup()
    {
        WiFi.mode(WIFI_STA);

        ssidEepromAddress = eepromUtils::reserveSpace(max_ssid_size);
        passwordEepromAddress = eepromUtils::reserveSpace(max_password_size);
        autoConnectEepromAddress = eepromUtils::reserveSpace(sizeof(bool));

        if (eepromUtils::firstInitialized)
        {
            saveToEEPROM();
        }
        else
        {
            loadFromEEPROM();
        }

        if (_autoConnect)
        {
            connect();
        }

        wifiServer::setup();
    }

    void connect()
    {
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

        setAutoConnect(false);
    }

    void loop()
    {
        wifiServer::loop();
    }
} // namespace wifi
