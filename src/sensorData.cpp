#include "definitions.h"
#include "sensorData.h"
#include "type.h"

namespace sensorData
{
    bool isValidSensorType(SensorType sensorType)
    {
        return sensorType < SensorType::COUNT;
    }

    const uint16_t min_delay_ms = 20;
    uint16_t motionConfiguration[(uint8_t)motionSensor::DataType::COUNT]{0};
    uint16_t pressureConfiguration[(uint8_t)pressureSensor::DataType::COUNT]{0};
    bool hasAtLeastOneNonzeroDelay = false;
    void updateHasAtLeastOneNonzeroDelay()
    {
        hasAtLeastOneNonzeroDelay = false;

        for (uint8_t i = 0; i < (uint8_t)motionSensor::DataType::COUNT && !hasAtLeastOneNonzeroDelay; i++)
        {
            if (motionConfiguration[i] != 0)
            {
                hasAtLeastOneNonzeroDelay = true;
            }
        }

        for (uint8_t i = 0; i < (uint8_t)pressureSensor::DataType::COUNT && !hasAtLeastOneNonzeroDelay; i++)
        {
            if (pressureConfiguration[i] != 0)
            {
                hasAtLeastOneNonzeroDelay = true;
            }
        }
    }
    void setConfiguration(const uint8_t *newConfiguration, uint8_t size, SensorType sensorType)
    {
        for (uint8_t offset = 0; offset < size; offset += 3)
        {
            auto sensorDataTypeIndex = newConfiguration[offset];
            uint16_t delay = ((uint16_t)newConfiguration[offset + 2] << 8) | newConfiguration[offset + 1];
            delay -= (delay % min_delay_ms);

#if DEBUG
            Serial.print("setting delay: ");
            Serial.print(delay);
            Serial.print(" for sensor type: ");
            Serial.print((uint8_t)sensorType);
            Serial.print(" for sensor data type: ");
            Serial.print(sensorDataTypeIndex);
            Serial.println("...");
#endif

            switch (sensorType)
            {
            case SensorType::MOTION:
                if (motionSensor::isValidDataType((motionSensor::DataType)sensorDataTypeIndex))
                {
                    motionConfiguration[sensorDataTypeIndex] = delay;
                }
                else
                {
#if DEBUG
                    Serial.println("invalid sensor data type");
#endif
                }
                break;
            case SensorType::PRESSURE:
                if (pressureSensor::isValidDataType((pressureSensor::DataType)sensorDataTypeIndex))
                {
                    pressureConfiguration[sensorDataTypeIndex] = delay;
                }
                else
                {
#if DEBUG
                    Serial.println("invalid sensor data type");
#endif
                }
                break;
            default:
#if DEBUG
                Serial.println("invalid sensor type");
#endif
                break;
            }
        }
    }
    void setConfigurations(const uint8_t *newConfigurations, uint8_t size)
    {
        uint8_t offset = 0;
        while (offset < size)
        {
            const auto sensorType = (SensorType)newConfigurations[offset++];
            if (!isValidSensorType(sensorType))
            {
#if DEBUG
                Serial.print("invalid sensor type: ");
                Serial.println((uint8_t)sensorType);
#endif
                break;
            }
            const uint8_t _size = newConfigurations[offset++];
            setConfiguration(&newConfigurations[offset], _size, sensorType);
            offset += _size;
        }

        updateHasAtLeastOneNonzeroDelay();
    }
    void clearConfiguration(SensorType sensorType)
    {
        switch (sensorType)
        {
        case SensorType::MOTION:
            memset(motionConfiguration, 0, sizeof(motionConfiguration));
            break;
        case SensorType::PRESSURE:
            memset(pressureConfiguration, 0, sizeof(pressureConfiguration));
            break;
        default:
            break;
        }
    }
    void clearConfigurations()
    {
        for (uint8_t index = 0; index < (uint8_t)SensorType::COUNT; index++)
        {
            auto sensorType = (SensorType)index;
            clearConfiguration(sensorType);
        }

        updateHasAtLeastOneNonzeroDelay();
    }

    uint8_t motionData[(uint8_t)motionSensor::DataSize::TOTAL]{0};
    uint8_t motionDataSize = 0;
    uint8_t pressureData[(uint8_t)pressureSensor::DataSize::TOTAL]{0};
    uint8_t pressureDataSize = 0;
    void clearMotionData()
    {
        memset(motionData, 0, sizeof(motionData));
        motionDataSize = 0;
    }
    void updateMotionData()
    {
        clearMotionData();
        for (uint8_t dataTypeIndex = 0; dataTypeIndex < (uint8_t)motionSensor::DataType::COUNT; dataTypeIndex++)
        {
            uint16_t delay = motionConfiguration[dataTypeIndex];
            if (delay != 0 && (lastDataUpdateTime % delay == 0))
            {
                auto dataType = (motionSensor::DataType)dataTypeIndex;

                int16_t data[4];
                uint8_t dataSize = 0;
                switch (dataType)
                {
                case motionSensor::DataType::ACCELERATION:
                    motionSensor::bno.getRawVectorData(Adafruit_BNO055::VECTOR_ACCELEROMETER, data);
                    dataSize = (uint8_t)motionSensor::DataSize::ACCELERATION;
                    break;
                case motionSensor::DataType::GRAVITY:
                    motionSensor::bno.getRawVectorData(Adafruit_BNO055::VECTOR_GRAVITY, data);
                    dataSize = (uint8_t)motionSensor::DataSize::GRAVITY;
                    break;
                case motionSensor::DataType::LINEAR_ACCELERATION:
                    motionSensor::bno.getRawVectorData(Adafruit_BNO055::VECTOR_LINEARACCEL, data);
                    dataSize = (uint8_t)motionSensor::DataSize::LINEAR_ACCELERATION;
                    break;
                case motionSensor::DataType::ROTATION_RATE:
                    motionSensor::bno.getRawVectorData(Adafruit_BNO055::VECTOR_GYROSCOPE, data);
                    dataSize = (uint8_t)motionSensor::DataSize::ROTATION_RATE;
                    break;
                case motionSensor::DataType::MAGNETOMETER:
                    motionSensor::bno.getRawVectorData(Adafruit_BNO055::VECTOR_MAGNETOMETER, data);
                    dataSize = (uint8_t)motionSensor::DataSize::MAGNETOMETER;
                    break;
                case motionSensor::DataType::QUATERNION:
                    motionSensor::bno.getRawQuatData(data);
                    dataSize = (uint8_t)motionSensor::DataSize::QUATERNION;
                    break;
                default:
                    Serial.print("uncaught motion data type: ");
                    Serial.println(dataTypeIndex);
                    break;
                }

                if (dataSize > 0)
                {
                    motionData[motionDataSize++] = dataTypeIndex;
                    memcpy(&motionData[motionDataSize], data, dataSize);
                    motionDataSize += dataSize;
                }
            }
        }
    }
    void clearPressureData()
    {
        memset(pressureData, 0, sizeof(pressureData));
        pressureDataSize = 0;
    }
    void updatePressureData()
    {
        clearPressureData();

        bool didUpdateSensor = false;
        for (uint8_t dataTypeIndex = 0; dataTypeIndex < (uint8_t)pressureSensor::DataType::COUNT; dataTypeIndex++)
        {
            uint16_t delay = pressureConfiguration[dataTypeIndex];
            if (delay != 0 && (lastDataUpdateTime % delay == 0))
            {
                if (!didUpdateSensor)
                {
                    pressureSensor::update();
                    didUpdateSensor = true;
                }

                auto dataType = (pressureSensor::DataType)dataTypeIndex;

                uint8_t *data = nullptr;
                uint8_t dataSize = 0;

                switch (dataType)
                {
                case pressureSensor::DataType::SINGLE_BYTE:
                    dataSize = (uint8_t)pressureSensor::DataSize::SINGLE_BYTE;
                    data = pressureSensor::getPressureDataSingleByte();
                    break;
                case pressureSensor::DataType::DOUBLE_BYTE:
                    dataSize = (uint8_t)pressureSensor::DataSize::DOUBLE_BYTE;
                    data = (uint8_t *)pressureSensor::getPressureDataDoubleByte();
                    break;
                case pressureSensor::DataType::CENTER_OF_MASS:
                    dataSize = (uint8_t)pressureSensor::DataSize::CENTER_OF_MASS;
                    data = (uint8_t *)pressureSensor::getCenterOfMass();
                    break;
                case pressureSensor::DataType::MASS:
                {
                    dataSize = (uint8_t)pressureSensor::DataSize::MASS;
                    data = (uint8_t *)pressureSensor::getMass();
#if DEBUG
                    Serial.print("PACKING MASS: ");
                    for (auto i = 0; i < dataSize; i++)
                    {
                        Serial.print(data[i]);
                        Serial.print(", ");
                    }
                    Serial.println();
#endif
                }
                break;
                case pressureSensor::DataType::HEEL_TO_TOE:
                {
                    dataSize = (uint8_t)pressureSensor::DataSize::HEEL_TO_TOE;
                    data = (uint8_t *)pressureSensor::getHeelToToe();
#if DEBUG
                    Serial.print("PACKING HEEL To tOE: ");
                    for (auto i = 0; i < dataSize; i++)
                    {
                        Serial.print(data[i]);
                        Serial.print(", ");
                    }
                    Serial.println();
#endif
                }
                break;
                default:
                    Serial.print("uncaught pressure data type: ");
                    Serial.println(dataTypeIndex);
                    break;
                }

                if (dataSize > 0)
                {
                    pressureData[pressureDataSize++] = dataTypeIndex;
                    memcpy(&pressureData[pressureDataSize], data, dataSize);
                    pressureDataSize += dataSize;
                }
            }
        }
    }

    void updateData()
    {
        updateMotionData();
        if (type::isInsole())
        {
            updatePressureData();
        }
    }

    unsigned long currentTime;
    unsigned long lastDataUpdateTime;
    void loop()
    {
        currentTime = millis();
        if (hasAtLeastOneNonzeroDelay && currentTime >= lastDataUpdateTime + min_delay_ms)
        {
            lastDataUpdateTime = currentTime - (currentTime % min_delay_ms);
            updateData();
        }
    }
} // namespace sensorData