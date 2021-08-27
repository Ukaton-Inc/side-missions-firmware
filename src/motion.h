#pragma once
#ifndef _MOTION_
#define _MOTION_

#include <Adafruit_BNO055.h>
#include <lwipopts.h>

namespace motion
{
    extern Adafruit_BNO055 bno;
    extern bool isBnoAwake;
 
    extern const uint16_t calibration_delay_ms;
    typedef enum: uint8_t
    {
        SYSTEM_CALIBRATION = 0,
        ACCELEROMETER_CALIBRATION,
        GYROSCOPE_CALIBRATION,
        MAGNETOMETER_CALIBRATION,
        NUMBER_OF_CALIBRATION_TYPES = 4
    } CalibrationType;
    extern uint8_t callibration[NUMBER_OF_CALIBRATION_TYPES];
    void updateCalibration();
    void calibrationLoop();

    void loadFromEEPROM();
    bool saveToEEPROM();

    typedef enum: uint8_t
    {
        ACCELERATION = 0,
        GRAVITY,
        LINEAR_ACCELERATION,
        ROTATION_RATE,
        MAGNETOMETER,
        QUATERNION,
        NUMBER_OF_DATA_TYPES
    } DataType;
    
    extern const uint16_t data_delay_ms;
    extern uint16_t configuration[NUMBER_OF_DATA_TYPES];
    void setConfiguration(uint16_t *newConfiguration);
    extern unsigned long lastDataLoopTime;
    void dataLoop();
    void sendData();

    void setup();
    void start();
    void stop();

    extern unsigned long currentTime;
    void loop();
} // namespace motion

#endif // _MOTION_