#pragma once
#ifndef _SENSOR_DATA_
#define _SENSOR_DATA_

#include "motionSensor.h"
#include "pressureSensor.h"

#include <array>

namespace sensorData
{
    enum class SensorType : uint8_t
    {
        MOTION,
        PRESSURE,
        COUNT
    };

    struct Configurations {
        std::array<uint16_t, (uint8_t) motionSensor::DataType::COUNT> motion;
        std::array<uint16_t, (uint8_t) pressureSensor::DataType::COUNT> pressure;
        bool hasAtLeastOneNonzeroDelay = false;
    };
    extern Configurations configurations;

    extern uint16_t motionConfiguration[(uint8_t) motionSensor::DataType::COUNT];
    extern uint16_t pressureConfiguration[(uint8_t) pressureSensor::DataType::COUNT];
    void setConfigurations(const uint8_t *newConfigurations, uint8_t size, Configurations &_configurations = configurations);
    void clearConfigurations(Configurations &_configurations = configurations);
    bool isValidSensorType(SensorType sensorType);
    
    /*
    TODO - can later use a struct instead of arrays and stuff...
    struct Data {
        std::array<uint16_t, (uint8_t) motionSensor::DataType::COUNT> motion;
        std::array<uint16_t, (uint8_t) pressureSensor::DataType::COUNT> pressure;
    }
    */
    extern uint8_t motionData[(uint8_t)motionSensor::DataSize::TOTAL + (uint8_t)motionSensor::DataType::COUNT];
    extern uint8_t motionDataSize;
    extern uint8_t pressureData[(uint8_t)pressureSensor::DataSize::TOTAL + (uint8_t)pressureSensor::DataType::COUNT];
    extern uint8_t pressureDataSize;

    extern unsigned long lastDataUpdateTime;
    constexpr uint16_t min_delay_ms = 20;
    void loop();
} // namespace sensorData

#endif // _SENSOR_DATA_