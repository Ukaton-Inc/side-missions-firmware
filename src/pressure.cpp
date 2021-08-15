#include "pressure.h"
#include "ble.h"

namespace pressure
{
    const uint16_t data_delay_ms = 20;
    const uint8_t data_base_offset = sizeof(uint8_t) + sizeof(unsigned long);

    uint16_t delays[NUMBER_OF_DATA_TYPES] = {0};
    BLECharacteristic *pConfigurationCharacteristic;
    class ConfigurationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t length = pCharacteristic->getDataLength();
            if (length == NUMBER_OF_DATA_TYPES * sizeof(uint16_t))
            {
                uint16_t *characteristicData = (uint16_t *) pCharacteristic->getValue().data();
                for (uint8_t i = 0; i < NUMBER_OF_DATA_TYPES; i++)
                {
                    delays[i] = characteristicData[i];
                    delays[i] -= delays[i] % data_delay_ms;
                }
            }
            pCharacteristic->setValue((uint8_t *)delays, sizeof(delays));
        }
    };

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
            if (delays[SINGLE_BYTE] != 0)
            {
                pressureDataHalf[pressureSensorIndex] = pressureData[pressureSensorIndex] >> 4;
            }
            if (delays[MASS] != 0 || delays[CENTER_OF_MASS] != 0 || delays[HEEL_TO_TOE] != 0)
            {
                mass += pressureData[pressureSensorIndex];
            }
        }

        if (delays[CENTER_OF_MASS] != 0)
        {
            centerOfMass[0] = 0;
            centerOfMass[1] = 0;

            if (mass > 0) {
                for (uint8_t pressureSensorIndex = 0; pressureSensorIndex < number_of_pressure_sensors; pressureSensorIndex++)
                {
                    centerOfMass[0] += (float)(sensorPositions[pressureSensorIndex][0] * (float)pressureData[pressureSensorIndex]) / (float)mass;
                    centerOfMass[1] += (float)(sensorPositions[pressureSensorIndex][1] * (float)pressureData[pressureSensorIndex]) / (float)mass;
                }
            }

        }

        if (delays[HEEL_TO_TOE] != 0) {
            heelToToe = 0;
            
            if (mass > 0) {
                for (uint8_t pressureSensorIndex = 0; pressureSensorIndex < number_of_pressure_sensors; pressureSensorIndex++)
                {
                    heelToToe += (double)(sensorPositions[pressureSensorIndex][1] * (double)pressureData[pressureSensorIndex]) / (double)mass;
                }
            }
        }
    }

    BLECharacteristic *pDataCharacteristic;
    uint8_t dataCharacteristicValue[ble::max_characteristic_value_length];
    uint8_t dataCharacteristicValueBitmask;
    uint8_t dataCharacteristicValueOffset;
    uint8_t dataCharacteristicValueBuffer[ble::max_characteristic_value_length];
    unsigned long lastDataLoopTime;
    bool hasAtLeastOneNonzeroDelay()
    {
        bool _hasAtLeastOneNonzeroDelay = false;
        for (uint8_t i = 0; i < NUMBER_OF_DATA_TYPES && !_hasAtLeastOneNonzeroDelay; i++)
        {
            if (delays[i] != 0)
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
            updateData();
            lastDataLoopTime = currentTime - (currentTime % data_delay_ms);
        }
    }
    void updateData()
    {
        dataCharacteristicValueBitmask = 0;
        dataCharacteristicValueOffset = data_base_offset;
        for (uint8_t i = 0; i < NUMBER_OF_DATA_TYPES; i++)
        {
            if (delays[i] != 0 && lastDataLoopTime % delays[i] == 0)
            {
                switch (i)
                {
                case SINGLE_BYTE:
                    addData(i, SINGLE_BYTE_BIT_INDEX);
                    break;
                case DOUBLE_BYTE:
                    addData(i, DOUBLE_BYTE_0_BIT_INDEX);
                    // delay(10);
                    addData(i, DOUBLE_BYTE_1_BIT_INDEX);
                    break;
                case CENTER_OF_MASS:
                    addData(i, CENTER_OF_MASS_BIT_INDEX);
                    break;
                case MASS:
                    addData(i, MASS_BIT_INDEX);
                    break;
                case HEEL_TO_TOE:
                    addData(i, HEEL_TO_TOE_BIT_INDEX);
                    break;
                }
            }
        }

        if (dataCharacteristicValueBitmask != 0)
        {
            sendData();
        }
    }
    void addData(uint8_t dataType, uint8_t bitIndex)
    {
        uint8_t size = 0;
        switch (dataType)
        {
        case SINGLE_BYTE:
            size = SINGLE_BYTE_SIZE;
            MEMCPY(&dataCharacteristicValueBuffer, &pressureDataHalf, size);
            break;
        case DOUBLE_BYTE:
            switch (bitIndex)
            {
            case DOUBLE_BYTE_0_BIT_INDEX:
                size = DOUBLE_BYTE_0_SIZE;
                MEMCPY(&dataCharacteristicValueBuffer, &pressureData, size);
                break;
            case DOUBLE_BYTE_1_BIT_INDEX:
                size = DOUBLE_BYTE_1_SIZE;
                MEMCPY(&dataCharacteristicValueBuffer, &pressureData[8], size);
                break;
            }
            break;
        case CENTER_OF_MASS:
            size = CENTER_OF_MASS_SIZE;
            MEMCPY(&dataCharacteristicValueBuffer, &centerOfMass, size);
            break;
        case MASS:
            size = MASS_SIZE;
            MEMCPY(&dataCharacteristicValueBuffer, &mass, size);
            break;
        case HEEL_TO_TOE:
            size = HEEL_TO_TOE_SIZE;
            MEMCPY(&dataCharacteristicValueBuffer, &heelToToe, size);
            break;
        }

        if (dataCharacteristicValueOffset + size > ble::max_characteristic_value_length)
        {
            sendData();
            // delay(10);
        }

        bitSet(dataCharacteristicValueBitmask, bitIndex);
        MEMCPY(&dataCharacteristicValue[dataCharacteristicValueOffset], &dataCharacteristicValueBuffer, size);
        dataCharacteristicValueOffset += size;
    }
    void sendData()
    {
        MEMCPY(&dataCharacteristicValue[0], &dataCharacteristicValueBitmask, 1);
        MEMCPY(&dataCharacteristicValue[1], &currentTime, sizeof(currentTime));
        pDataCharacteristic->setValue((uint8_t *)dataCharacteristicValue, dataCharacteristicValueOffset);
        pDataCharacteristic->notify();

        memset(&dataCharacteristicValue, 0, sizeof(dataCharacteristicValue));
        dataCharacteristicValueBitmask = 0;
        dataCharacteristicValueOffset = data_base_offset;
    }

    void setup()
    {
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

        pConfigurationCharacteristic = ble::createCharacteristic(GENERATE_UUID("5000"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "pressure configuration");
        pConfigurationCharacteristic->setValue((uint8_t *)delays, sizeof(delays));
        pConfigurationCharacteristic->setCallbacks(new ConfigurationCharacteristicCallbacks());

        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("5001"), NIMBLE_PROPERTY::NOTIFY, "pressure data");
    }
    void stop()
    {
        memset(&delays, 0, sizeof(delays));
        pConfigurationCharacteristic->setValue((uint8_t *)delays, sizeof(delays));
    }

    unsigned long currentTime;
    void loop()
    {
        currentTime = millis();
        if (ble::isServerConnected)
        {
            dataLoop();
        }
    }
} // namespace pressure