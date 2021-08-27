#include "name.h"
#include "eepromUtils.h"

namespace name
{
    const uint8_t MAX_NAME_LENGTH = 30;

    std::string name = "Ukaton Motion Module";
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

    void setName(char *newName)
    {
        if (strlen(newName) <= MAX_NAME_LENGTH) {
            Serial.print("changing name to: ");
            Serial.println(newName);
            name = newName;
            saveToEEPROM();
        }
        else {
            log_e("name's too long");
        }
    }
} // namespace name