#include "debug.h"
#include "eepromUtils.h"
#include "definitions.h"

namespace debug
{
    const uint8_t MAX_NAME_LENGTH = 30;

    bool isEnabled = DEBUG;
    uint16_t eepromAddress;

    void loadFromEEPROM() {
        isEnabled = EEPROM.readBool(eepromAddress);
    }
    void saveToEEPROM() {
        Serial.print("Saving debugging enabled to EEPROM: ");
        Serial.println(isEnabled);
        EEPROM.writeBool(eepromAddress, isEnabled);
        EEPROM.commit();
    }

    void setup()
    {
        eepromAddress = eepromUtils::reserveSpace(sizeof(isEnabled));

        if (eepromUtils::firstInitialized) {
            saveToEEPROM();
        }
        else {
            loadFromEEPROM();
        }
    }

    bool getEnabled() {
        return isEnabled;
    }
    void setEnabled(bool _isEnabled)
    {
        isEnabled = _isEnabled;
        saveToEEPROM();
        Serial.print("changed isEnabled to: ");
        Serial.println(isEnabled);
    }
} // namespace debug