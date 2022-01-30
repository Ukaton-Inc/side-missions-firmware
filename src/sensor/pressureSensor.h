#pragma once
#ifndef _PRESSURE_SENSOR_
#define _PRESSURE_SENSOR_

#include <Arduino.h>
#include <lwipopts.h>

#include "pressureSensor.h"

namespace pressureSensor
{
    extern bool isRightInsole;

    constexpr uint8_t number_of_pressure_sensors = 16;
    enum class DataType : uint8_t
    {
        SINGLE_BYTE,
        DOUBLE_BYTE,
        CENTER_OF_MASS,
        MASS,
        HEEL_TO_TOE,
        COUNT
    };
    bool isValidDataType(DataType dataType);
    enum class DataSize : uint8_t
    {
        SINGLE_BYTE = sizeof(uint8_t) * 16,
        DOUBLE_BYTE = sizeof(uint16_t) * 16,
        CENTER_OF_MASS = 2 * sizeof(float),
        MASS = sizeof(uint32_t),
        HEEL_TO_TOE = sizeof(double),
        TOTAL = (sizeof(uint8_t) * 16) + (sizeof(uint16_t) * 16) + (2 * sizeof(double)) + sizeof(uint32_t) + sizeof(uint32_t)
    };

    void setup();
    void setupPins();
    extern bool isRightInsole;
    void updateSide(bool isRightInsole);
    void update();

    uint8_t *const getPressureDataSingleByte();
    uint16_t *const getPressureDataDoubleByte();
    float *const getCenterOfMass();
    uint32_t *const getMass();
    double *const getHeelToToe();
} // namespace pressureSensor

#endif // _PRESSURE_SENSOR_