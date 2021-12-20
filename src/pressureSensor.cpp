#include "pressureSensor.h"
#include "definitions.h"
#include "eepromUtils.h"
#include "type.h"

#include <Arduino.h>
#include <map>

namespace pressureSensor
{

    bool isValidDataType(DataType dataType)
    {
        return dataType < DataType::COUNT;
    }

    bool isRightInsole;
    const uint8_t dataPin = 36;
    const uint8_t configurationPins[4] = {32, 33, 26, 25};
    const uint8_t bothPinConfigurations[2][number_of_pressure_sensors][4] = {
        // RIGHT INSOLE
        {{0, 0, 0, 0},
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
         {1, 1, 1, 0}},

        // LEFT INSOLE
        {{1, 1, 1, 1},
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
         {0, 0, 0, 1}}};

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

    void setup()
    {
        if (type::isInsole()) {
            updateSide(type::isRightInsole());
        }
    }
    void setupPins()
    {
        for (uint8_t i = 0; i < 4; i++)
        {
            pinMode(configurationPins[i], OUTPUT);
        }
    }
    void updateSide(bool _isRightInsole)
    {        
        isRightInsole = _isRightInsole;

        for (uint8_t i = 0; i < number_of_pressure_sensors; i++)
        {
            sensorPositions[i][0] /= 93.257;
            if (isRightInsole)
            {
                sensorPositions[i][0] = 1 - sensorPositions[i][0];
            }
            sensorPositions[i][1] /= 265.069;
        }

        setupPins();
    }

    uint8_t pressureDataSingleByte[number_of_pressure_sensors]{0};
    uint16_t pressureDataDoubleByte[number_of_pressure_sensors]{0};
    float centerOfMass[2];
    uint32_t mass = 0;
    double heelToToe;

    std::map<DataType, bool> didUpdate;
    void update()
    {
        uint8_t pinConfigurationIndex = isRightInsole? 0:1;
        for (uint8_t pressureSensorIndex = 0; pressureSensorIndex < number_of_pressure_sensors; pressureSensorIndex++)
        {
            for (uint8_t configurationPinIndex = 0; configurationPinIndex < 4; configurationPinIndex++)
            {
                digitalWrite(configurationPins[configurationPinIndex], bothPinConfigurations[pinConfigurationIndex][pressureSensorIndex][configurationPinIndex]);
            }
            pressureDataDoubleByte[pressureSensorIndex] = analogRead(dataPin);
        }
        didUpdate.clear();
    }

    uint8_t *const getPressureDataSingleByte()
    {
        if (!didUpdate[DataType::SINGLE_BYTE])
        {
            for (uint8_t index = 0; index < number_of_pressure_sensors; index++)
            {
                pressureDataSingleByte[index] = pressureDataDoubleByte[index] >> 4;
            }
            didUpdate[DataType::SINGLE_BYTE] = true;
        }
        return pressureDataSingleByte;
    }

    uint16_t *const getPressureDataDoubleByte()
    {
        return pressureDataDoubleByte;
    }

    float *const getCenterOfMass()
    {
        if (!didUpdate[DataType::CENTER_OF_MASS])
        {
            centerOfMass[0] = 0;
            centerOfMass[1] = 0;

            auto mass = (*getMass());

            if (mass > 0)
            {
                for (uint8_t index = 0; index < number_of_pressure_sensors; index++)
                {
                    centerOfMass[0] += (float)(sensorPositions[index][0] * (float)pressureDataDoubleByte[index]) / (float)mass;
                    centerOfMass[1] += (float)(sensorPositions[index][1] * (float)pressureDataDoubleByte[index]) / (float)mass;
                }
            }

            didUpdate[DataType::CENTER_OF_MASS] = true;
        }
        return centerOfMass;
    }

    uint32_t *const getMass()
    {
        if (!didUpdate[DataType::MASS])
        {
            mass = 0;
            for (uint8_t index = 0; index < number_of_pressure_sensors; index++)
            {
                mass += pressureDataDoubleByte[index];
            }
            didUpdate[DataType::MASS] = true;
        }
        return &mass;
    }

    double *const getHeelToToe()
    {
        if (!didUpdate[DataType::HEEL_TO_TOE])
        {
            heelToToe = 0;

            auto mass = (*getMass());
            if (mass > 0)
            {
                for (uint8_t index = 0; index < number_of_pressure_sensors; index++)
                {
                    heelToToe += (double)(sensorPositions[index][1] * (double)pressureDataDoubleByte[index]) / (double)mass;
                }
            }

            didUpdate[DataType::HEEL_TO_TOE] = true;
        }
        return &heelToToe;
    }
} // namespace pressureSensor