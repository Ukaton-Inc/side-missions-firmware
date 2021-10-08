#pragma once
#ifndef _MOTION_
#define _MOTION_

#include <Adafruit_BNO055.h>
#include "driver/adc.h"

namespace motion
{
    enum class CalibrationType : uint8_t
    {
        SYSTEM,
        ACCELEROMETER,
        GYROSCOPE,
        MAGNETOMETER,
        COUNT
    };

    enum class DataType : uint8_t
    {
        ACCELERATION,
        GRAVITY,
        LINEAR_ACCELERATION,
        ROTATION_RATE,
        MAGNETOMETER,
        QUATERNION,
        COUNT
    };

    extern Adafruit_BNO055 bno;

    void setup();
    void loadFromEEPROM();
    bool saveToEEPROM();
    
    void loop();

    constexpr auto interrupt_pin = GPIO_NUM_27;
    extern unsigned long lastTimeMoved;

    extern uint8_t calibration[(uint8_t) CalibrationType::COUNT];
} // namespace motion

#endif // _MOTION_