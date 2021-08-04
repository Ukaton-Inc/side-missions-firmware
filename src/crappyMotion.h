#pragma once
#ifndef _CRAPPY_MOTION_
#define _CRAPPY_MOTION_

#include "ble.h"
#include <lwipopts.h>
#include <MPU9250.h>

namespace crappyMotion
{
    extern MPU9250 mpu;

    extern const uint16_t data_delay_ms;
    extern const uint8_t data_base_offset;
 
    typedef enum: uint8_t
    {
        CALIBRATING_ACCELEROMETER_AND_GYROSCOPE = 0,
        CALIBRATING_MAGNETOMETER,
        DONE_CALIBRATING
    } CalibrationStatuses;
    extern uint8_t calibrationStatus;
    extern BLECharacteristic *pCalibrateCharacteristic;
    extern BLECharacteristic *pCalibrationStatusCharacteristic;
    class CalibrateCharacteristicCallbacks;
    extern bool isCalibrating;
    void calibrate(uint8_t calibrationType);

    typedef enum: uint8_t
    {
        ACCELERATION = 0,
        ROTATION_RATE,
        MAGNETOMETER,
        QUATERNION,
        NUMBER_OF_DATA_TYPES = 4
    } DataType;

    extern uint16_t delays[NUMBER_OF_DATA_TYPES];
    extern BLECharacteristic *pConfigurationCharacteristic;
    class ConfigurationCharacteristicCallbacks;

    extern BLECharacteristic *pDataCharacteristic;
    extern uint8_t dataCharacteristicValue[ble::max_characteristic_value_length]; // [timestamp, bitmask, values]
    extern uint8_t dataCharacteristicValueBitmask;
    extern uint8_t dataCharacteristicValueOffset;
    extern int16_t rawData[7];
    extern uint8_t dataCharacteristicValueBuffer[ble::max_characteristic_value_length];
    extern unsigned long lastDataLoopTime;
    void updateData();
    void addData(uint8_t dataType, uint8_t bitIndex);
    void sendData();
    void dataLoop();

    void setup();
    void stop();

    extern unsigned long currentTime;
    void loop();
} // namespace crappyMotion

#endif // _CRAPPY_MOTION_