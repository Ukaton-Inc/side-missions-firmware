#pragma once
#ifndef _MOTION_
#define _MOTION_

#include <Adafruit_BNO055.h>
#include <lwipopts.h>
#include "ble.h"

namespace motion
{
    extern Adafruit_BNO055 bno;
    extern bool isBnoAwake;

    extern const uint16_t calibration_delay_ms;
    extern const uint8_t data_delay_ms;
    extern const uint8_t data_base_offset;
 
    typedef enum: uint8_t
    {
        SYSTEM_CALIBRATION = 0,
        ACCELEROMETER_CALIBRATION,
        GYROSCOPE_CALIBRATION,
        MAGNETOMETER_CALIBRATION,
        NUMBER_OF_CALIBRATION_TYPES = 4
    } CalibrationType;
    extern BLECharacteristic *pCalibrationCharacteristic;
    extern uint8_t callibration[NUMBER_OF_CALIBRATION_TYPES];
    void updateCalibration();
    extern unsigned long lastCalibrationLoopTime;
    void calibrationLoop();

    typedef enum: uint16_t
    {
        ACCELEROMETER_RANGE = 4000 + 1000,
        GYROSCOPE_RANGE = 32000 + 1000,
        MAGNETOMETER_RANGE = 6400 + 960
    } DataRange;

    typedef enum: uint8_t
    {
        ACCELERATION = 0,
        GRAVITY,
        LINEAR_ACCELERATION,
        ROTATION_RATE,
        MAGNETOMETER,
        QUATERNION,
        NUMBER_OF_DATA_TYPES = 6
    } DataType;

    extern uint16_t delays[NUMBER_OF_DATA_TYPES];
    extern BLECharacteristic *pConfigurationCharacteristic;
    class ConfigurationCharacteristicCallbacks;

    extern BLECharacteristic *pDataCharacteristic;
    extern uint8_t dataCharacteristicValue[ble::max_characteristic_value_length]; // [timestamp, sensorId, values]
    extern uint8_t dataCharacteristicValueBitmask;
    extern uint8_t dataCharacteristicValueOffset;
    extern int16_t dataCharacteristicValueBuffer[4];
    extern unsigned long lastDataLoopTime;
    void updateData();
    void addData(uint8_t bitIndex, bool isVector = false, Adafruit_BNO055::adafruit_vector_type_t vector_type = Adafruit_BNO055::VECTOR_ACCELEROMETER);
    void sendData();
    void dataLoop();

    void setup();
    void start();
    void stop();

    extern unsigned long currentTime;
    void loop();
} // namespace motion

#endif // _MOTION_