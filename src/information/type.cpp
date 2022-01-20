#include "type.h"
#include "definitions.h"
#include <Preferences.h>
#include "definitions.h"
#include "sensor/pressureSensor.h"
#include "weight/weightData.h"

namespace type
{
    Type type = IS_INSOLE ? (IS_RIGHT_INSOLE ? Type::RIGHT_INSOLE : Type::LEFT_INSOLE) : Type::MOTION_MODULE;
    bool _isInsole = false;
    bool _isRightInsole = false;

    void onTypeUpdate()
    {
        _isInsole = (type == Type::LEFT_INSOLE) || (type == Type::RIGHT_INSOLE);
        if (_isInsole)
        {
            _isRightInsole = (type == Type::RIGHT_INSOLE);
            pressureSensor::updateSide(_isRightInsole);
        }
        else {
            weightData::setDelay(0);
        }
    }

    Preferences preferences;
    void setup()
    {
        preferences.begin("type");
        type = (Type) preferences.getUChar("type", (uint8_t) type);
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
        preferences.putUChar("type", (uint8_t) type);
        Serial.print("changed device type to: ");
        Serial.println((uint8_t)type);
        onTypeUpdate();
    }
} // namespace type