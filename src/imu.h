#pragma once
#ifndef _IMU_
#define _IMU_

#include <Adafruit_BNO055.h>
#include <lwipopts.h>
#include "ble.h"

#define IMU_CALIBRATION_DELAY_MS (1000)
#define IMU_DATA_DELAY_MS (20)
#define IMU_DATA_BASE_OFFSET sizeof(uint8_t) + sizeof(unsigned long)

namespace imu
{
    extern Adafruit_BNO055 bno;
    extern bool isBnoAwake;

    enum CalibrationType: uint8_t
    {
        SYSTEM_CALIBRATION = 0,
        ACCELEROMETER_CALIBRATION,
        GYROSCOPE_CALIBRATION,
        MAGNETOMETER_CALIBRATION,
        NUMBER_OF_CALIBRATION_TYPES = 4
    };
    extern BLECharacteristic *pCalibrationCharacteristic;
    extern uint8_t callibration[NUMBER_OF_CALIBRATION_TYPES];
    void updateCalibration();
    extern unsigned long lastCalibrationLoopTime;
    void calibrationLoop();

    enum DataType: uint8_t
    {
        ACCELERATION = 0,
        GRAVITY,
        LINEAR_ACCELERATION,
        ROTATION_RATE,
        MAGNETOMETER,
        QUATERNION,
        NUMBER_OF_DATA_TYPES = 6
    };

    extern uint16_t delays[NUMBER_OF_DATA_TYPES];
    extern BLECharacteristic *pConfigurationCharacteristic;
    class ConfigurationCharacteristicCallbacks;

    extern BLECharacteristic *pDataCharacteristic;
    extern uint8_t dataCharacteristicValue[MAX_CHARACTERISTIC_VALUE_LENGTH]; // [timestamp, sensorId, values]
    extern uint8_t dataCharacteristicValueBitmask;
    extern uint8_t dataCharacteristicValueOffset;
    extern uint8_t dataCharacteristicValueBuffer[8];
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
}

#endif // _IMU_