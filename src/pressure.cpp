#include "pressure.h"
#include "definitions.h"

#include <Arduino.h>

namespace pressure
{

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

    void setup()
    {
#if IS_INSOLE
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
#endif
    }
} // namespace pressure