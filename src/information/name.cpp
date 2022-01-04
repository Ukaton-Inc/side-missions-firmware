#include "name.h"
#include "eepromUtils.h"
#include "definitions.h"
#include "services/gap/ble_svc_gap.h"
#include <WiFi.h>

namespace name
{
    std::string name = DEFAUlT_NAME;
    uint16_t eepromAddress;

    void loadFromEEPROM() {
        name = EEPROM.readString(eepromAddress).c_str();
    }
    void saveToEEPROM() {
        Serial.print("Saving name to EEPROM: ");
        Serial.println(name.c_str());
        EEPROM.writeString(eepromAddress, name.substr(0, min((const uint8_t)name.length(), MAX__NAME_LENGTH)).c_str());
        EEPROM.commit();
    }

    void setup()
    {
        eepromAddress = eepromUtils::reserveSpace(MAX__NAME_LENGTH);

        if (eepromUtils::firstInitialized) {
            saveToEEPROM();
        }
        else {
            loadFromEEPROM();
        }

        Serial.print("name: ");
        Serial.println(name.c_str());

        ble_svc_gap_device_name_set(name.c_str());
        WiFi.setHostname(name.c_str());
    }

    const std::string *getName() {
        return &name;
    }
    void setName(const char *newName, size_t length)
    {
        if (length <= MAX__NAME_LENGTH) {
            name.assign(newName, length);
            saveToEEPROM();
            Serial.print("changed name to: ");
            Serial.println(name.c_str());
            ble_svc_gap_device_name_set(name.c_str());
            WiFi.setHostname(name.c_str());
        }
        else {
            log_e("name's too long");
        }
    }
    void setName(const char *newName)
    {
        uint8_t length = strlen(newName);
        setName(newName, length);
    }
} // namespace name