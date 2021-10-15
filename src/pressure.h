#pragma once
#ifndef _PRESSURE_
#define _PRESSURE_

#include <Arduino.h>
#include <lwipopts.h>

namespace pressure
{
    const uint8_t number_of_pressure_sensors = 16;

    enum class DataType : uint8_t
    {
        SINGLE_BYTE,
        DOUBLE_BYTE,
        CENTER_OF_MASS,
        MASS,
        HEEL_TO_TOE,
        COUNT
    };

    void setup();

    void update();

    uint8_t *getPressureDataSingleByte();
    uint16_t *getPressureDataDoubleByte();
    float *getCenterOfMass();
    uint32_t *const getMass();
    double *const getHeelToToe();
} // namespace pressure

#endif // _PRESSURE_