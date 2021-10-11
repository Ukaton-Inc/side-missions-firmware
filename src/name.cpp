#include "name.h"
#include "eepromUtils.h"
#include "definitions.h"

namespace name
{
    const uint8_t MAX_NAME_LENGTH = 30;

    std::string name = NAME;
    uint16_t eepromAddress;

    void loadFromEEPROM() {
        name = EEPROM.readString(eepromAddress).c_str();
    }
    void saveToEEPROM() {
        Serial.print("Saving name to EEPROM: ");
        Serial.println(name.c_str());
        EEPROM.writeString(eepromAddress, name.substr(0, min((const uint8_t)name.length(), MAX_NAME_LENGTH)).c_str());
        EEPROM.commit();
    }

    void setup()
    {
        eepromAddress = eepromUtils::reserveSpace(MAX_NAME_LENGTH);

        if (eepromUtils::firstInitialized) {
            saveToEEPROM();
        }
        else {
            loadFromEEPROM();
        }
    }

    const std::string *getName() {
        return &name;
    }
    void setName(const char *newName, size_t length)
    {
        if (length <= MAX_NAME_LENGTH) {
            name.assign(newName, length);
            saveToEEPROM();
            Serial.print("changed name to: ");
            Serial.println(name.c_str());
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