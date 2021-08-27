#include "pressure.h"
#include "websocketServer.h"

namespace pressure
{
    const uint16_t data_delay_ms = 20;
    const uint8_t data_base_offset = sizeof(uint8_t) + sizeof(unsigned long);

    uint16_t configuration[NUMBER_OF_DATA_TYPES] = {0};
    void setConfiguration(uint16_t *newConfiguration)
    {
        for (uint8_t dataType = 0; dataType < NUMBER_OF_DATA_TYPES; dataType++)
        {
            configuration[dataType] = newConfiguration[dataType];
            configuration[dataType] -= configuration[dataType] % data_delay_ms;
        }
    }

    uint16_t pressureData[number_of_pressure_sensors];
    uint8_t pressureDataHalf[number_of_pressure_sensors];
    float centerOfMass[2];
    uint32_t mass;
    double heelToToe;

    uint8_t dataPin = 36;
    uint8_t configurationPins[4] = {32, 33, 26, 25};
    uint8_t pinConfigurations[number_of_pressure_sensors][4] = {
#if IS_RIGHT_INSOLE

        {0, 0, 0, 0},
        {1, 0, 1, 1},

        {1, 0, 0, 0},
        {0, 1, 1, 1},
        {0, 0, 1, 1},

        {0, 1, 0, 0},
        {1, 1, 1, 1},
        {1, 1, 0, 1},

        {1, 1, 0, 0},
        {0, 1, 0, 1},

        {0, 0, 1, 0},
        {1, 0, 0, 1},

        {1, 0, 1, 0},
        {0, 0, 0, 1},

        {0, 1, 1, 0},
        {1, 1, 1, 0}
#else
        {1, 1, 1, 1},
        {0, 1, 0, 0},

        {0, 1, 1, 1},
        {1, 0, 0, 0},
        {1, 1, 0, 0},

        {1, 0, 1, 1},
        {0, 0, 0, 0},
        {0, 0, 1, 0},

        {0, 0, 1, 1},
        {1, 0, 1, 0},

        {1, 1, 0, 1},
        {0, 1, 1, 0},

        {0, 1, 0, 1},
        {1, 1, 1, 0},

        {1, 0, 0, 1},
        {0, 0, 0, 1}
#endif
    };

    double sensorPositions[number_of_pressure_sensors][2] = {
        {59.55, 32.3},
        {33.1, 42.15},

        {69.5, 55.5},
        {44.11, 64.8},
        {20.3, 71.9},

        {63.8, 81.1},
        {41.44, 90.8},
        {19.2, 102.8},

        {48.3, 119.7},
        {17.8, 130.5},

        {43.3, 177.7},
        {18.0, 177.0},

        {43.3, 200.6},
        {18.0, 200.0},

        {43.5, 242.0},
        {18.55, 242.1}};

    void
    getData()
    {
        mass = 0;
        for (uint8_t pressureSensorIndex = 0; pressureSensorIndex < number_of_pressure_sensors; pressureSensorIndex++)
        {
            for (uint8_t configurationPinIndex = 0; configurationPinIndex < 4; configurationPinIndex++)
            {
                digitalWrite(configurationPins[configurationPinIndex], pinConfigurations[pressureSensorIndex][configurationPinIndex]);
            }
            pressureData[pressureSensorIndex] = analogRead(dataPin);
            if (configuration[SINGLE_BYTE] != 0)
            {
                pressureDataHalf[pressureSensorIndex] = pressureData[pressureSensorIndex] >> 4;
            }
            if (configuration[MASS] != 0 || configuration[CENTER_OF_MASS] != 0 || configuration[HEEL_TO_TOE] != 0)
            {
                mass += pressureData[pressureSensorIndex];
            }
        }

        if (configuration[CENTER_OF_MASS] != 0)
        {
            centerOfMass[0] = 0;
            centerOfMass[1] = 0;

            if (mass > 0)
            {
                for (uint8_t pressureSensorIndex = 0; pressureSensorIndex < number_of_pressure_sensors; pressureSensorIndex++)
                {
                    centerOfMass[0] += (float)(sensorPositions[pressureSensorIndex][0] * (float)pressureData[pressureSensorIndex]) / (float)mass;
                    centerOfMass[1] += (float)(sensorPositions[pressureSensorIndex][1] * (float)pressureData[pressureSensorIndex]) / (float)mass;
                }
            }
        }

        if (configuration[HEEL_TO_TOE] != 0)
        {
            heelToToe = 0;

            if (mass > 0)
            {
                for (uint8_t pressureSensorIndex = 0; pressureSensorIndex < number_of_pressure_sensors; pressureSensorIndex++)
                {
                    heelToToe += (double)(sensorPositions[pressureSensorIndex][1] * (double)pressureData[pressureSensorIndex]) / (double)mass;
                }
            }
        }
    }

    unsigned long lastDataLoopTime;
    bool hasAtLeastOneNonzeroDelay()
    {
        bool _hasAtLeastOneNonzeroDelay = false;
        for (uint8_t i = 0; i < NUMBER_OF_DATA_TYPES && !_hasAtLeastOneNonzeroDelay; i++)
        {
            if (configuration[i] != 0)
            {
                _hasAtLeastOneNonzeroDelay = true;
            }
        }
        return _hasAtLeastOneNonzeroDelay;
    }
    void dataLoop()
    {
        if (hasAtLeastOneNonzeroDelay() && currentTime >= lastDataLoopTime + data_delay_ms)
        {
            getData();
            sendData();
            lastDataLoopTime = currentTime - (currentTime % data_delay_ms);
        }
    }
    void sendData()
    {
        uint8_t dataSize = 0;
        uint8_t maxDataSize = 0;
        uint8_t dataBitmask = 0;
        for (uint8_t dataType = 0; dataType < NUMBER_OF_DATA_TYPES; dataType++)
        {
            if (configuration[dataType] != 0 && lastDataLoopTime % configuration[dataType] == 0)
            {
                bitSet(dataBitmask, dataType);

                switch (dataType)
                    {
                    case SINGLE_BYTE:
                        dataSize += SINGLE_BYTE_SIZE;
                        maxDataSize = max(maxDataSize, (uint8_t) SINGLE_BYTE_SIZE);
                        break;
                    case DOUBLE_BYTE:
                        dataSize += DOUBLE_BYTE_0_SIZE + DOUBLE_BYTE_1_SIZE;
                        maxDataSize = max(maxDataSize, (uint8_t) (DOUBLE_BYTE_0_SIZE + DOUBLE_BYTE_1_SIZE));
                        break;
                    case CENTER_OF_MASS:
                        dataSize += CENTER_OF_MASS_SIZE;
                        maxDataSize = max(maxDataSize, (uint8_t) CENTER_OF_MASS_SIZE);
                        break;
                    case MASS:
                        dataSize += MASS_SIZE;
                        maxDataSize = max(maxDataSize, (uint8_t) MASS_SIZE);
                        break;
                    case HEEL_TO_TOE:
                        dataSize += HEEL_TO_TOE_SIZE;
                        maxDataSize = max(maxDataSize, (uint8_t) HEEL_TO_TOE_SIZE);
                        break;
                    default:
                        break;
                    }
            }
        }

        if (dataSize > 0)
        {
            uint8_t dataOffset = 1 + 1 + 4;
            uint8_t data[dataOffset + dataSize];
            data[0] = websocketServer::PRESSURE_DATA;
            data[1] = dataBitmask;
            MEMCPY(&data[2], &currentTime, sizeof(currentTime));

            int16_t _data[maxDataSize] = {0};
            for (uint8_t dataType = 0; dataType < NUMBER_OF_DATA_TYPES; dataType++)
            {
                if (bitRead(dataBitmask, dataType))
                {
                    uint8_t _dataSize = 0;

                    switch (dataType)
                    {
                    case SINGLE_BYTE:
                        _dataSize = SINGLE_BYTE_SIZE;
                        break;
                    case DOUBLE_BYTE:
                        _dataSize = DOUBLE_BYTE_0_SIZE + DOUBLE_BYTE_1_SIZE;
                        break;
                    case CENTER_OF_MASS:
                        _dataSize = MASS_SIZE;
                        break;
                    case MASS:
                        _dataSize = MASS_SIZE;
                        break;
                    case HEEL_TO_TOE:
                        _dataSize = HEEL_TO_TOE_SIZE;
                        break;
                    default:
                        break;
                    }

                    if (_dataSize > 0)
                    {
                        MEMCPY(&data[dataOffset], &_data, _dataSize);
                        dataOffset += _dataSize;
                    }
                }
            }
            websocketServer::ws.binaryAll(data, sizeof(data));
        }
    }

    void setup()
    {
        #if !IS_INSOLE
            return;
        #endif

        for (uint8_t i = 0; i < 4; i++)
        {
            pinMode(configurationPins[i], OUTPUT);
        }
        for (uint8_t i = 0; i < 4; i++)
        {
            digitalWrite(configurationPins[i], LOW);
        }

        for (uint8_t i = 0; i < number_of_pressure_sensors; i++)
        {
            sensorPositions[i][0] /= 93.257;
#if IS_RIGHT_INSOLE
            sensorPositions[i][0] = 1 - sensorPositions[i][0];
#endif
            sensorPositions[i][1] /= 265.069;
        }
    }
    void stop()
    {
        memset(&configuration, 0, sizeof(configuration));
    }

    unsigned long currentTime;
    void loop()
    {
        currentTime = millis();
        if (IS_INSOLE && websocketServer::ws.count() > 0)
        {
            dataLoop();
        }
    }
} // namespace pressure