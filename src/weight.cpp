#include "weight.h"
#include "pressure.h"
#include "definitions.h"

#if IS_RIGHT_INSOLE
    #include "weight_model_right.h"
#else
    #include "weight_model_left.h"
#endif

namespace weight
{
    bool isModelLoaded = false;
    uint8_t interpreterBuffer[sizeof(tflite::MicroInterpreter)];
    alignas(16) byte tensorArena[tensorArenaSize];

    tflite::MicroErrorReporter tflErrorReporter;
    tflite::AllOpsResolver tflOpsResolver;

    const tflite::Model *tflModel = nullptr;
    tflite::MicroInterpreter *tflInterpreter = nullptr;
    TfLiteTensor *tflInputTensor = nullptr;
    TfLiteTensor *tflOutputTensor = nullptr;

    BLECharacteristic *pInferenceCharacteristic;
    BLECharacteristic *pMakeInferenceCharacteristic;

    bool makeInferenceFlag = false;

    unsigned long currentTime;

    unsigned char* weight_model;
    unsigned int weight_model_length;

    class MakeInferenceCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *data = (uint8_t *) pCharacteristic->getValue().data();
            uint8_t makeInference = data[0];
            if (makeInference == 1)
            {
                makeInferenceFlag = true;
            }
            Serial.print("set weight make inference: ");
            Serial.println(makeInference);
        }
    };

    void setup()
    {
        pInferenceCharacteristic = ble::createCharacteristic(GENERATE_UUID("6000"), NIMBLE_PROPERTY::NOTIFY, "Weight Model Inference");
        pMakeInferenceCharacteristic = ble::createCharacteristic(GENERATE_UUID("6001"), NIMBLE_PROPERTY::WRITE, "Make Weight Model Inference");

        pMakeInferenceCharacteristic->setCallbacks(new MakeInferenceCharacteristicCallbacks());

         #if IS_RIGHT_INSOLE
            weight_model = weight_model_right;
            weight_model_length = weight_model_right_length;
        #else
            weight_model = weight_model_left;
            weight_model_length = weight_model_left_length;
        #endif

        if (weight_model_length > 0)
        {
            loadModel(weight_model);
        }
    }

    void loadModel(unsigned char model[])
    {
        tflModel = tflite::GetModel(model);
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

    void loop()
    {
        currentTime = millis();
        if (ble::isServerConnected && isModelLoaded && makeInferenceFlag)
        {
            makeInferenceFlag = false;
            makeInference();
        }
    }

    void makeInference() {
        pressure::getData();
        for (uint8_t sensorIndex = 0; sensorIndex < pressure::number_of_pressure_sensors; sensorIndex++)
        {
            tflInputTensor->data.f[sensorIndex] = (float)pressure::pressureData[sensorIndex] / (float)4096;
        }

        TfLiteStatus invokeStatus = tflInterpreter->Invoke();
        if (invokeStatus != kTfLiteOk)
        {
            Serial.println("Invoke failed!");
            return;
        }

        float weight = tflOutputTensor->data.f[0];
        uint8_t *weightArray = reinterpret_cast<uint8_t *>(&weight);
        uint8_t *currentTimeArray = reinterpret_cast<uint8_t *>(&currentTime);
        uint8_t inferenceCharacteristicData[sizeof(weight) + sizeof(currentTime)] = {weightArray[0], weightArray[1], weightArray[2], weightArray[3], currentTimeArray[0], currentTimeArray[1], currentTimeArray[2], currentTimeArray[3]};
        pInferenceCharacteristic->setValue(inferenceCharacteristicData, sizeof(inferenceCharacteristicData));
        if (ble::isServerConnected)
        {
            pInferenceCharacteristic->notify();
        }
    }
} // namespace weight