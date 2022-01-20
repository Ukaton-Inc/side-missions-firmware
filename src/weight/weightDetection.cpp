#include "weightDetection.h"
#include "sensor/pressureSensor.h"
#include <Weight_Detection_inferencing.h>

namespace weightDetection
{
    float features[pressureSensor::number_of_pressure_sensors]{};
    void updateFeatures() {
        pressureSensor::update();
        auto pressureData = pressureSensor::getPressureDataDoubleByte();
        for (uint8_t index = 0; index < pressureSensor::number_of_pressure_sensors; index++) {
            features[index] = pressureData[index];
        }
    }

    int raw_feature_get_data(size_t offset, size_t length, float *out_ptr)
    { 
        memcpy(out_ptr, features + offset, length * sizeof(float));
        return 0;
    }

    float guessWeight()
    {
        ei_impulse_result_t result = {0};

        updateFeatures();

        // the features are stored into flash, and we don't want to load everything into RAM
        signal_t features_signal;
        features_signal.total_length = sizeof(features) / sizeof(features[0]);
        features_signal.get_data = &raw_feature_get_data;

        // invoke the impulse
        EI_IMPULSE_ERROR res = run_classifier(&features_signal, &result, false /* debug */);

        if (res != 0)
            return -1;

        Serial.print("result: ");
        Serial.println(result.classification[0].value);
        return result.classification[0].value;
    }
} // namespace weightDetection
