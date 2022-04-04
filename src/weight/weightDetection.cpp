#include "definitions.h"
#include "weightDetection.h"
#include "sensor/pressureSensor.h"
#include "information/type.h"

#include <FS.h>
#ifdef USE_LittleFS
#define SPIFFS LITTLEFS
#include <LITTLEFS.h>
#else
#include <SPIFFS.h>
#endif

#undef DEFAULT
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

namespace weightDetection
{
    bool isModelLoaded = false;
    uint8_t interpreterBuffer[sizeof(tflite::MicroInterpreter)];
    constexpr int tensorArenaSize = 8 * 1024;
    alignas(16) byte tensorArena[tensorArenaSize];

    tflite::MicroErrorReporter tflErrorReporter;
    tflite::AllOpsResolver tflOpsResolver;

    const tflite::Model *tflModel = nullptr;
    tflite::MicroInterpreter *tflInterpreter = nullptr;
    TfLiteTensor *tflInputTensor = nullptr;
    TfLiteTensor *tflOutputTensor = nullptr;

    File tfliteFile;
    bool loadedTfliteFile = false;
    void getModelFile() {
        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            loadedTfliteFile = false;
        }
        if (SPIFFS.exists(filePath.c_str())) {
            tfliteFile = SPIFFS.open(filePath.c_str(), FILE_READ);
            loadedTfliteFile = true;
        }
        else {
            loadedTfliteFile = false;
        }
    }

    uint8_t *loadedModel;
    void getModel() {
        getModelFile();
        if (!loadedTfliteFile) {
            return;
        }

        size_t modelSize = tfliteFile.size();
        Serial.printf("Found model on filesystem of size %u\n", modelSize);

        free(loadedModel);
        loadedModel = (uint8_t*) malloc(modelSize);
        tfliteFile.readBytes((char *) loadedModel, modelSize);
        tfliteFile.close();
        SPIFFS.end();
    }

    void loadModel(bool override) {
        if (type::isInsole() && (!isModelLoaded || override)) {
            getModel();
            if (!loadedTfliteFile) {
                isModelLoaded = false;
                return;
            }

            tflModel = tflite::GetModel(loadedModel);
            if (tflModel->version() != TFLITE_SCHEMA_VERSION)
            {
                Serial.println("Model schema mismatch!");
            }

            tflInterpreter = new (interpreterBuffer) tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

            tflInterpreter->AllocateTensors();

            tflInputTensor = tflInterpreter->input(0);
            tflOutputTensor = tflInterpreter->output(0);

            isModelLoaded = true;
            Serial.println("loaded model");
        }
    }

    float features[pressureSensor::number_of_pressure_sensors]{};
    void updateFeatures() {
        pressureSensor::update();
        auto pressureData = pressureSensor::getPressureDataDoubleByte();
        for (uint8_t index = 0; index < pressureSensor::number_of_pressure_sensors; index++) {
            features[index] = pressureData[index];
        }
    }

    float guessWeight()
    {
        if (!isModelLoaded) {
            return -1;
        }

        //Serial.println("updating features");
        updateFeatures();
        for (uint8_t sensorIndex = 0; sensorIndex < pressureSensor::number_of_pressure_sensors; sensorIndex++)
        {
            tflInputTensor->data.f[sensorIndex] = features[sensorIndex];
        }
        //Serial.println("updasted tensor");

        TfLiteStatus invokeStatus = tflInterpreter->Invoke();
        if (invokeStatus != kTfLiteOk)
        {
            Serial.println("Invoke failed!");
            return -1;
        }

        float weight = tflOutputTensor->data.f[0];
        //Serial.printf("weight: %f\n", weight);

        return weight;
    }
} // namespace weightDetection
