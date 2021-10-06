#pragma once
#ifndef _MOTION_
#define _MOTION_

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

    void setup();
    void loadFromEEPROM();
    bool saveToEEPROM();
    
    void loop();

    constexpr auto interrupt_pin = GPIO_NUM_27;
} // namespace motion

#endif // _MOTION_