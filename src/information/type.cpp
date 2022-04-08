#include "type.h"
#include "definitions.h"
#include <Preferences.h>
#include "definitions.h"
#include "sensor/pressureSensor.h"
#include "weight/weightData.h"
#include "weight/weightDetection.h"
#include "haptics.h"

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
            weightDetection::loadModel();
            haptics::setup();
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
        preferences.end();
        
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
    bool isTypeValid(Type newType) {
        return ((uint8_t) newType) < ((uint8_t) Type::COUNT);
    }
    void setType(Type newType)
    {
        if (isTypeValid(newType)) {
            type = newType;
            preferences.begin("type");
            preferences.putUChar("type", (uint8_t) type);
            preferences.end();
            Serial.print("changed device type to: ");
            Serial.println((uint8_t)type);
            onTypeUpdate();
        }
        else {
            log_e("invalid type to set");
        }
    }
} // namespace type