#pragma once
#ifndef _MOTION_SENSOR_
#define _MOTION_SENSOR_

//#include "SparkFun_BNO080_Arduino_Library.h"
#include <Adafruit_BNO055.h>
#include "driver/adc.h"

namespace motionSensor
{
    //extern BNO080 _bno;
    extern Adafruit_BNO055 bno;

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
    bool isValidDataType(DataType dataType);
    enum class DataSize : uint8_t
    {
        ACCELERATION = 6,
        GRAVITY = 6,
        LINEAR_ACCELERATION = 6,
        ROTATION_RATE = 6,
        MAGNETOMETER = 6,
        QUATERNION = 8,
        TOTAL = 38
    };

    enum class CalibrationType : uint8_t
    {
        SYSTEM,
        ACCELEROMETER,
        GYROSCOPE,
        MAGNETOMETER,
        COUNT
    };
    extern uint8_t calibration[(uint8_t) CalibrationType::COUNT];
    extern unsigned long lastCalibrationUpdateTime;

    constexpr auto interrupt_pin = GPIO_NUM_27;
    extern unsigned long lastTimeMoved;

    void setup();
    void loop();
} // namespace motionSensor

#endif // _MOTION_SENSOR_