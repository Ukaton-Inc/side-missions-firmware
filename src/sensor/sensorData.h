#pragma once
#ifndef _SENSOR_DATA_
#define _SENSOR_DATA_

#include "motionSensor.h"
#include "pressureSensor.h"

namespace sensorData
{
    enum class SensorType : uint8_t
    {
        MOTION,
        PRESSURE,
        COUNT
    };
    
    extern uint16_t motionConfiguration[(uint8_t) motionSensor::DataType::COUNT];
    extern uint16_t pressureConfiguration[(uint8_t) pressureSensor::DataType::COUNT];
    void setConfigurations(const uint8_t *newConfigurations, uint8_t size);
    void clearConfigurations();
    bool isValidSensorType(SensorType sensorType);
    
    extern uint8_t motionData[(uint8_t)motionSensor::DataSize::TOTAL + (uint8_t)motionSensor::DataType::COUNT];
    extern uint8_t motionDataSize;
    extern uint8_t pressureData[(uint8_t)pressureSensor::DataSize::TOTAL + (uint8_t)pressureSensor::DataType::COUNT];
    extern uint8_t pressureDataSize;

    extern unsigned long lastDataUpdateTime;
    constexpr uint16_t min_delay_ms = 20;
    void loop();
} // namespace sensorData

#endif // _SENSOR_DATA_