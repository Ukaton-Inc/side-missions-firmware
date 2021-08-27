#pragma once
#ifndef _PRESSURE_
#define _PRESSURE_

#include <lwipopts.h>

#define IS_INSOLE false
#define IS_RIGHT_INSOLE true

namespace pressure
{
    extern const uint16_t data_delay_ms;
    extern const uint8_t data_base_offset;
    const uint8_t number_of_pressure_sensors = 16;

    typedef enum: uint8_t
    {
        SINGLE_BYTE = 0,
        DOUBLE_BYTE,
        CENTER_OF_MASS,
        MASS,
        HEEL_TO_TOE,
        NUMBER_OF_DATA_TYPES = 6
    } DataType;

    typedef enum: uint16_t
    {
        SINGLE_BYTE_RANGE = 256,
        DOUBLE_BYTE_RANGE = 4096
    } DataRange;

    typedef enum: uint8_t
    {
        SINGLE_BYTE_BIT_INDEX = 0,
        DOUBLE_BYTE_0_BIT_INDEX,
        DOUBLE_BYTE_1_BIT_INDEX,
        CENTER_OF_MASS_BIT_INDEX,
        MASS_BIT_INDEX,
        HEEL_TO_TOE_BIT_INDEX
    } DataBitIndex;

    typedef enum: uint8_t
    {
        SINGLE_BYTE_SIZE = 16,
        DOUBLE_BYTE_0_SIZE = 16,
        DOUBLE_BYTE_1_SIZE = 16,
        CENTER_OF_MASS_SIZE = 8,
        MASS_SIZE = 4,
        HEEL_TO_TOE_SIZE = 8
    } DataSize;

    extern uint16_t configuration[NUMBER_OF_DATA_TYPES];
    void setConfiguration(uint16_t *newConfiguration);

    extern uint16_t pressureData[number_of_pressure_sensors];
    extern float centerOfMass[2];
    extern uint32_t mass;
    extern double heelToToe;
    void getData();

    extern unsigned long lastDataLoopTime;
    void dataLoop();
    void sendData();

    void setup();
    void stop();

    extern unsigned long currentTime;
    void loop();
} // namespace pressure

#endif // _PRESSURE_