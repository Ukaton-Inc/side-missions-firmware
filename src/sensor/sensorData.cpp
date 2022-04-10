#include "definitions.h"
#include "sensorData.h"
#include "information/type.h"

#include "wifi/webSocket.h"
#include "ble/ble.h"
#include "wifi/udp.h"

namespace sensorData
{
    bool isValidSensorType(SensorType sensorType)
    {
        return sensorType < SensorType::COUNT;
    }

    Configurations configurations;
    void updateHasAtLeastOneNonzeroDelay(Configurations &_configurations)
    {
        _configurations.hasAtLeastOneNonzeroDelay = false;

        for (uint8_t i = 0; i < (uint8_t)motionSensor::DataType::COUNT && !_configurations.hasAtLeastOneNonzeroDelay; i++)
        {
            if (_configurations.motion[i] != 0)
            {
                _configurations.hasAtLeastOneNonzeroDelay = true;
            }
        }

        for (uint8_t i = 0; i < (uint8_t)pressureSensor::DataType::COUNT && !_configurations.hasAtLeastOneNonzeroDelay; i++)
        {
            if (_configurations.pressure[i] != 0)
            {
                _configurations.hasAtLeastOneNonzeroDelay = true;
            }
        }
    }
    void setConfiguration(const uint8_t *newConfiguration, uint8_t size, SensorType sensorType, Configurations &_configurations)
    {
        for (uint8_t offset = 0; offset < size; offset += 3)
        {
            auto sensorDataTypeIndex = newConfiguration[offset];
            uint16_t delay = ((uint16_t)newConfiguration[offset + 2] << 8) | (uint16_t)newConfiguration[offset + 1];
            delay -= (delay % min_delay_ms);

            switch (sensorType)
            {
            case SensorType::MOTION:
                if (motionSensor::isValidDataType((motionSensor::DataType)sensorDataTypeIndex))
                {
                    _configurations.motion[sensorDataTypeIndex] = delay;
                    switch (motionSensor::DataType)
                    {
                        case motionSensor::DataType::ACCELERATION:
                        motionSensor::bno.enableAccelerometer(delay);
                        break;
                        case motionSensor::DataType::GRAVITY:
                        motionSensor::bno.enableGravity(delay);
                        break;
                        case motionSensor::DataType::LINEAR_ACCELERATION:
                        motionSensor::bno.enableLinearAccelerometer(delay);
                        break;
                        case motionSensor::DataType::ROTATION_RATE:
                        motionSensor::bno.enableGyro(delay);
                        break;
                        case motionSensor::DataType::MAGNETOMETER:
                        motionSensor::bno.enableMagnetometer(delay);
                        break;
                        case motionSensor::DataType::QUATERNION:
                        motionSensor::bno.enableRotationVector(delay);
                        break;
                        default:
                        Serial.printf("uncaught motion sensor type %u\n", sensorDataTypeIndex);
                        break;
                    }
                }
                break;
            case SensorType::PRESSURE:
                if (pressureSensor::isValidDataType((pressureSensor::DataType)sensorDataTypeIndex))
                {
                    _configurations.pressure[sensorDataTypeIndex] = delay;
                }
                break;
            default:
                break;
            }
        }

        if (sensorType == SensorType::PRESSURE)
        {
            if (_configurations.pressure[(uint8_t)pressureSensor::DataType::SINGLE_BYTE] > 0 && _configurations.pressure[(uint8_t)pressureSensor::DataType::DOUBLE_BYTE] > 0)
            {
                _configurations.pressure[(uint8_t)pressureSensor::DataType::SINGLE_BYTE] = 0;
            }
        }
    }
    void setConfigurations(const uint8_t *newConfigurations, uint8_t size, Configurations &_configurations)
    {
        uint8_t offset = 0;
        while (offset < size)
        {
            const auto sensorType = (SensorType)newConfigurations[offset++];
            if (isValidSensorType(sensorType))
            {
                const uint8_t _size = newConfigurations[offset++];
                setConfiguration(&newConfigurations[offset], _size, sensorType, _configurations);
                offset += _size;
            }
            else
            {
                Serial.printf("invalid sensor type: %d\n", (uint8_t)sensorType);
                break;
            }
        }

        flattenConfigurations(_configurations);

        updateHasAtLeastOneNonzeroDelay(_configurations);
    }
    void clearConfiguration(SensorType sensorType, Configurations &_configurations = configurations)
    {
        switch (sensorType)
        {
        case SensorType::MOTION:
            _configurations.motion.fill(0);
            motionSensor::bno.enableAccelerometer(0);
            motionSensor::bno.enableGravity(0);
            motionSensor::bno.enableLinearAccelerometer(0);
            motionSensor::bno.enableGyro(0);
            motionSensor::bno.enableMagnetometer(0);
            motionSensor::bno.enableRotationVector(0);
            break;
        case SensorType::PRESSURE:
            _configurations.pressure.fill(0);
            break;
        default:
            break;
        }
    }
    void clearConfigurations(Configurations &_configurations)
    {
        for (uint8_t index = 0; index < (uint8_t)SensorType::COUNT; index++)
        {
            auto sensorType = (SensorType)index;
            clearConfiguration(sensorType);
        }

        updateHasAtLeastOneNonzeroDelay(_configurations);
    }

    void flattenConfigurations(Configurations &_configurations)
    {
        std::copy(_configurations.motion.cbegin(), _configurations.motion.cend(), _configurations.flattened.begin());
        std::copy(_configurations.pressure.cbegin(), _configurations.pressure.cend(), _configurations.flattened.begin() + _configurations.motion.max_size());
    }

    uint8_t motionData[(uint8_t)motionSensor::DataSize::TOTAL + (uint8_t)motionSensor::DataType::COUNT]{0};
    uint8_t motionDataSize = 0;
    uint8_t pressureData[(uint8_t)pressureSensor::DataSize::TOTAL + (uint8_t)pressureSensor::DataType::COUNT]{0};
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
            const uint16_t delay = configurations.motion[dataTypeIndex];
            if (delay != 0 && ((lastDataUpdateTime % delay) == 0))
            {
                auto dataType = (motionSensor::DataType)dataTypeIndex;

                int16_t data[4];
                uint8_t dataSize = 0;
                switch (dataType)
                {
                case motionSensor::DataType::ACCELERATION:
                    motionSensor::bno.getRawAccel(data);
                    dataSize = (uint8_t)motionSensor::DataSize::ACCELERATION;
                    break;
                case motionSensor::DataType::GRAVITY:
                    motionSensor::bno.getRawGrav(data);
                    dataSize = (uint8_t)motionSensor::DataSize::GRAVITY;
                    break;
                case motionSensor::DataType::LINEAR_ACCELERATION:
                    motionSensor::bno.getRawLinAccel(data);
                    dataSize = (uint8_t)motionSensor::DataSize::LINEAR_ACCELERATION;
                    break;
                case motionSensor::DataType::ROTATION_RATE:
                    motionSensor::bno.getRawGyro(data);
                    dataSize = (uint8_t)motionSensor::DataSize::ROTATION_RATE;
                    break;
                case motionSensor::DataType::MAGNETOMETER:
                    motionSensor::bno.getRawMag(data);
                    dataSize = (uint8_t)motionSensor::DataSize::MAGNETOMETER;
                    break;
                case motionSensor::DataType::QUATERNION:
                    motionSensor::bno.getRawQuat(data);
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
            const uint16_t delay = configurations.pressure[dataTypeIndex];
            if (delay != 0 && ((lastDataUpdateTime % delay) == 0))
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
                }
                break;
                case pressureSensor::DataType::HEEL_TO_TOE:
                {
                    dataSize = (uint8_t)pressureSensor::DataSize::HEEL_TO_TOE;
                    data = (uint8_t *)pressureSensor::getHeelToToe();
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
        if (configurations.hasAtLeastOneNonzeroDelay && currentTime >= lastDataUpdateTime + min_delay_ms && (ble::isServerConnected || webSocket::isConnectedToClient() || udp::hasListener()))
        {
            updateData();
            lastDataUpdateTime = currentTime - (currentTime % min_delay_ms);
        }
    }
} // namespace sensorData