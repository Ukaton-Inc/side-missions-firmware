#pragma once
#ifndef _MOTION_
#define _MOTION_

#include <Adafruit_BNO055.h>
#include <lwipopts.h>

namespace motion
{
    extern Adafruit_BNO055 bno;
    extern bool isBnoAwake;
 
    typedef enum: uint8_t
    {
        SYSTEM_CALIBRATION = 0,
        ACCELEROMETER_CALIBRATION,
        GYROSCOPE_CALIBRATION,
        MAGNETOMETER_CALIBRATION,
        NUMBER_OF_CALIBRATION_TYPES = 4
    } CalibrationType;
    void updateCalibration();
    void calibrationLoop();

    void loadFromEEPROM();
    bool saveToEEPROM();

    void dataLoop();

    void setup();
    void start();
    void stop();

    extern unsigned long currentTime;
    void loop();
} // namespace motion

#endif // _MOTION_