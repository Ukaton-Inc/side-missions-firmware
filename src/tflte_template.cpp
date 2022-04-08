#include "definitions.h"
#include "tflite_template.h"

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

namespace tflite_template
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
        if (!isModelLoaded || override) {
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

    constexpr uint8_t numberOfFeatures = 1; // CHANGE
    float features[numberOfFeatures]{};
    void updateFeatures() {
        // CHANGE
        for (uint8_t index = 0; index < numberOfFeatures; index++) {
            features[index] = 0;
        }
    }

    constexpr uint8_t numberOfClasses = 1; // CHANGE

    void makeInference()
    {
        Serial.println("updating features");
        updateFeatures();
        for (uint8_t index = 0; index < sizeof(features); index++)
        {
            tflInputTensor->data.f[index] = features[index];
        }
        Serial.println("updated tensor");

        TfLiteStatus invokeStatus = tflInterpreter->Invoke();
        if (invokeStatus != kTfLiteOk)
        {
            Serial.println("Invoke failed!");
            return;
        }

        uint8_t maxIndex = 0;
        float maxValue = 0;
        for (uint8_t i = 0; i < numberOfClasses; i++)
        {
            float value = tflOutputTensor->data.f[i];
            Serial.printf("class: %u, score: %f", i, value);

            if (value > maxValue)
            {
                maxValue = value;
                maxIndex = i;
            }
        }
    }
} // namespace tflite_template
