#include "type.h"
#include "eepromUtils.h"
#include "definitions.h"
#include "pressureSensor.h"

namespace type
{
    Type type = Type::MOTION_MODULE;
    bool _isInsole = false;
    bool _isRightInsole = false;

    uint16_t eepromAddress;
    void loadFromEEPROM()
    {
        type = (Type)EEPROM.read(eepromAddress);
    }
    void saveToEEPROM()
    {
        Serial.print("Saving type to EEPROM: ");
        Serial.println((uint8_t)type);
        EEPROM.write(eepromAddress, (uint8_t)type);
        EEPROM.commit();
    }

    void onTypeUpdate() {
        _isInsole = (type == Type::LEFT_INSOLE) || (type == Type::RIGHT_INSOLE);
        if (_isInsole)
        {
            _isRightInsole = (type == Type::RIGHT_INSOLE);
            pressureSensor::updateSide(_isRightInsole);
        }
    }

    void setup()
    {
        eepromAddress = eepromUtils::reserveSpace(sizeof(type));

        if (eepromUtils::firstInitialized)
        {
            saveToEEPROM();
        }
        else
        {
            loadFromEEPROM();
        }

        onTypeUpdate();
    }

    Type getType()
    {
        return type;
    }
    bool isInsole()
    {
        return _isInsole;
    }
    bool isRightInsole()
    {
        return _isRightInsole;
    }
    void setType(Type newType)
    {
        type = newType;
        saveToEEPROM();
        Serial.print("changed device type to: ");
        Serial.println((uint8_t)type);
        onTypeUpdate();
    }
} // namespace type