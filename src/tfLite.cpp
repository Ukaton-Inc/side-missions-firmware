#include "tfLite.h"
#include "tfLite_model.h"

namespace tfLite
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

    bool enabled = false;
    uint8_t modelType = CLASSIFICATION; // modify
    uint8_t numberOfClasses = 2; // modify
    uint8_t dataTypesBitmask = 0b100; // modify
    uint16_t sampleRate = 40; // modify
    uint16_t numberOfSamples = 48; // modify
    float thresholds[NUMBER_OF_THRESHOLDS] = {8., 0}; // modify
    uint16_t captureDelay = 200; // modify

    bool isCapturingSamples = false;
    uint8_t sampleDataSize;
    uint16_t numberOfSamplesRead = 0;
    unsigned long currentTime;
    unsigned long lastUpdateLoopTime = 0;
    unsigned long lastCaptureTime = 0;

    BLECharacteristic *pHasModelCharacteristic;
    BLECharacteristic *pEnabledCharacteristic;
    BLECharacteristic *pModelTypeCharacteristic;
    BLECharacteristic *pNumberOfClassesCharacteristic;
    BLECharacteristic *pDataTypesCharacteristic;
    BLECharacteristic *pSampleRateCharacteristic;
    BLECharacteristic *pNumberOfSamplesCharacteristic;
    BLECharacteristic *pThresholdCharacteristic;
    BLECharacteristic *pCaptureDelayCharacteristic;
    BLECharacteristic *pInferenceCharacteristic;
    BLECharacteristic *pMakeInferenceCharacteristic;

    class EnabledCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *data = pCharacteristic->getData();
            enabled = data[0];
            Serial.print("set tflite enabled: ");
            Serial.println(enabled);
        }
    };
    class ModelTypeCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *data = pCharacteristic->getData();
            modelType = data[0];
            Serial.print("set tflite model type: ");
            Serial.println(modelType);
        }
    };
    class NumberOfClassesCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *data = pCharacteristic->getData();
            numberOfClasses = data[0];
            Serial.print("set tflite number of classes: ");
            Serial.println(numberOfClasses);
        }
    };
    class DataTypesCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint16_t *data = (uint16_t *)pCharacteristic->getData();
            dataTypesBitmask = data[0];
            Serial.print("set tflite data types: ");
            Serial.println(dataTypesBitmask);
        }
    };
    class SampleRateCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint16_t *data = (uint16_t *)pCharacteristic->getData();
            sampleRate = data[0];
            Serial.print("set tflite sample rate: ");
            Serial.println(sampleRate);
        }
    };
    class NumberOfSamplesCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint16_t *data = (uint16_t *)pCharacteristic->getData();
            numberOfSamples = data[0];
            Serial.print("set tflite number of samples: ");
            Serial.println(numberOfSamples);
        }
    };
    class ThresholdCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            float *data = (float *)pCharacteristic->getData();
            for (uint8_t i = 0; i < NUMBER_OF_THRESHOLDS; i++)
            {
                thresholds[i] = data[i];
            }
            
            Serial.print("set tflite thresholds: ");
            Serial.print(thresholds[0]);
            Serial.print(", ");
            Serial.println(thresholds[1]);
        }
    };
    class CaptureDelayCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint16_t *data = (uint16_t *)pCharacteristic->getData();
            captureDelay = data[0];
            Serial.print("set tflite capture delay: ");
            Serial.println(captureDelay);
        }
    };
    class MakeInferenceCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *data = pCharacteristic->getData();
            uint8_t makeInference = data[0];
            if (makeInference == 1) {
                startCapturingSamples();
            }
            Serial.print("set tflite make inference: ");
            Serial.println(makeInference);
        }
    };

    void setup()
    {
        pHasModelCharacteristic = ble::createCharacteristic(GENERATE_UUID("4000"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "Has TFlite Model");
        pEnabledCharacteristic = ble::createCharacteristic(GENERATE_UUID("4001"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "Enabled TFLite");
        pModelTypeCharacteristic = ble::createCharacteristic(GENERATE_UUID("4002"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "TFLite Model Type");
        pNumberOfClassesCharacteristic = ble::createCharacteristic(GENERATE_UUID("4003"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "Number of TFLite Classes");
        pDataTypesCharacteristic = ble::createCharacteristic(GENERATE_UUID("4004"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "TFLite Data Types");
        pSampleRateCharacteristic = ble::createCharacteristic(GENERATE_UUID("4005"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "TFLite Sample Rate");
        pNumberOfSamplesCharacteristic = ble::createCharacteristic(GENERATE_UUID("4006"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "Number of TFLite Samples");
        pThresholdCharacteristic = ble::createCharacteristic(GENERATE_UUID("4007"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "TFLite Threshold");
        pCaptureDelayCharacteristic = ble::createCharacteristic(GENERATE_UUID("4008"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "TFLite Capture Delay");
        pInferenceCharacteristic = ble::createCharacteristic(GENERATE_UUID("4009"), BLECharacteristic::PROPERTY_NOTIFY, "TFlite Model Inference");
        pMakeInferenceCharacteristic = ble::createCharacteristic(GENERATE_UUID("4010"), BLECharacteristic::PROPERTY_WRITE, "Make TFLite Model Inference");

        pEnabledCharacteristic->setCallbacks(new EnabledCharacteristicCallbacks());
        pModelTypeCharacteristic->setCallbacks(new ModelTypeCharacteristicCallbacks());
        pNumberOfClassesCharacteristic->setCallbacks(new NumberOfClassesCharacteristicCallbacks());
        pDataTypesCharacteristic->setCallbacks(new DataTypesCharacteristicCallbacks());
        pSampleRateCharacteristic->setCallbacks(new SampleRateCharacteristicCallbacks());
        pNumberOfSamplesCharacteristic->setCallbacks(new NumberOfSamplesCharacteristicCallbacks());
        pThresholdCharacteristic->setCallbacks(new ThresholdCharacteristicCallbacks());
        pCaptureDelayCharacteristic->setCallbacks(new CaptureDelayCharacteristicCallbacks());
        pMakeInferenceCharacteristic->setCallbacks(new MakeInferenceCharacteristicCallbacks());

        uint8_t pHasModelCharacteristicData[1] = {0};
        pHasModelCharacteristic->setValue(pHasModelCharacteristicData, sizeof(pHasModelCharacteristicData));
        uint8_t pEnabledCharacteristicData[1] = {0};
        pEnabledCharacteristic->setValue(pEnabledCharacteristicData, sizeof(pEnabledCharacteristicData));
        uint8_t pModelTypeCharacteristicData[1] = {modelType};
        pModelTypeCharacteristic->setValue(pModelTypeCharacteristicData, sizeof(pModelTypeCharacteristicData));
        uint8_t pNumberOfClassesCharacteristicData[1] = {numberOfClasses};
        pNumberOfClassesCharacteristic->setValue(pNumberOfClassesCharacteristicData, sizeof(pNumberOfClassesCharacteristicData));
        uint8_t pDataTypesCharacteristicData[1] = {0};
        pDataTypesCharacteristic->setValue(pDataTypesCharacteristicData, sizeof(pDataTypesCharacteristicData));
        uint16_t pSampleRateCharacteristicData[1] = {sampleRate};
        pSampleRateCharacteristic->setValue((uint8_t *)pSampleRateCharacteristicData, sizeof(pSampleRateCharacteristicData));
        uint16_t pNumberOfSamplesCharacteristicData[1] = {numberOfSamples};
        pNumberOfSamplesCharacteristic->setValue((uint8_t *)pNumberOfSamplesCharacteristicData, sizeof(pNumberOfSamplesCharacteristicData));
        pThresholdCharacteristic->setValue((uint8_t *)thresholds, sizeof(thresholds));
        uint16_t pCaptureDelayCharacteristicData[1] = {captureDelay};
        pCaptureDelayCharacteristic->setValue((uint8_t *)pCaptureDelayCharacteristicData, sizeof(pCaptureDelayCharacteristicData));

        if (tfLite_model_length > 0)
        {
            loadModel(tfLite_model);
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

        uint8_t pHasModelCharacteristicData[1] = {1};
        pHasModelCharacteristic->setValue(pHasModelCharacteristicData, sizeof(pHasModelCharacteristicData));
        if (ble::isServerConnected)
        {
            pHasModelCharacteristic->notify();
        }
    }
    void loop()
    {
        currentTime = millis();
        if (ble::isServerConnected && isModelLoaded && enabled)
        {
            updateLoop();
        }
    }
    void updateLoop()
    {
        if (sampleRate > 0 && dataTypesBitmask != 0 && currentTime >= lastUpdateLoopTime + sampleRate)
        {
            update();
            lastUpdateLoopTime = currentTime - (currentTime % sampleRate);
        }
    }
    void update()
    {
        if (!isCapturingSamples && captureDelay > 0 && (currentTime - lastCaptureTime) >= captureDelay)
        {
            bool reachedThresholds = true;

            for (uint8_t i = 0; i < NUMBER_OF_THRESHOLDS && reachedThresholds; i++)
            {
                if (thresholds[i] > 0)
                {
                    const float threshold = thresholds[i];
                    if (i == LINEAR_ACCELERATION_THRESHOLD)
                    {
                        imu::Vector<3> linearAccelerationVector = motion::bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
                        if (linearAccelerationVector.magnitude() < threshold)
                        {
                            reachedThresholds = false;
                        }
                    }
                    else if (i == ROTATION_RATE_THRESHOLD)
                    {
                        imu::Vector<3> rotationRateVector = motion::bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
                        if (rotationRateVector.magnitude() < threshold)
                        {
                            reachedThresholds = false;
                        }
                    }
                }
            }

            if (reachedThresholds)
            {
                Serial.println("REACHED THRESHOLD!");
                startCapturingSamples();
            }
        }

        if (isCapturingSamples)
        {
            int16_t buffer[4];
            uint8_t offset = 0;
            for (uint8_t i = 0; i < NUMBER_OF_DATA_TYPES; i++)
            {
                if (bitRead(dataTypesBitmask, i))
                {
                    uint8_t size = 0;
                    uint16_t range = 1;

                    switch(i) {
                        case ACCELERATION_BITMASK_INDEX:
                            motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_ACCELEROMETER, buffer);
                            size = 3;
                            range = motion::ACCELEROMETER_RANGE;
                            break;
                        case GRAVITY_BITMASK_INDEX:
                            motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_GRAVITY, buffer);
                            size = 3;
                            range = motion::ACCELEROMETER_RANGE;
                            break;
                        case LINEAR_ACCELERATION_BITMASK_INDEX:
                            motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_LINEARACCEL, buffer);
                            size = 3;
                            range = motion::ACCELEROMETER_RANGE;
                            break;
                        case ROTATION_RATE_BITMASK_INDEX:
                            motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_GYROSCOPE, buffer);
                            size = 3;
                            range = motion::GYROSCOPE_RANGE;
                            break;
                        case MAGNETOMETER_BITMASK_INDEX:
                            motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_MAGNETOMETER, buffer);
                            size = 3;
                            range = motion::MAGNETOMETER_RANGE;
                            break;
                        case QUATERNION_BITMASK_INDEX:
                            motion::bno.getRawQuatData(buffer);
                            size = 4;
                            range = 1;
                            break;
                    }

                    for (uint8_t i = 0; i < size; i++) {
                        tflInputTensor->data.f[(numberOfSamplesRead * sampleDataSize) + offset + i] = (float) buffer[i] / (float) range;
                    }
                    offset += size;
                }
            }

            if (++numberOfSamplesRead == numberOfSamples)
            {
                makeInference();
            }
        }
    }
    void startCapturingSamples() {
        if (isCapturingSamples) {
            return;
        }

        isCapturingSamples = true;
        numberOfSamplesRead = 0;

        sampleDataSize = 0;
        for (uint8_t i = 0; i < NUMBER_OF_DATA_TYPES; i++)
        {
            if (bitRead(dataTypesBitmask, i))
            {
                switch (i)
                {
                case ACCELERATION_BITMASK_INDEX:
                    sampleDataSize += 3;
                    break;
                case GRAVITY_BITMASK_INDEX:
                    sampleDataSize += 3;
                    break;
                case LINEAR_ACCELERATION_BITMASK_INDEX:
                    sampleDataSize += 3;
                    break;
                case ROTATION_RATE_BITMASK_INDEX:
                    sampleDataSize += 3;
                    break;
                case MAGNETOMETER_BITMASK_INDEX:
                    sampleDataSize += 3;
                    break;
                case QUATERNION_BITMASK_INDEX:
                    sampleDataSize += 4;
                    break;
                }
            }
        }
    }
    void makeInference()
    {
        isCapturingSamples = false;

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
            float _value = tflOutputTensor->data.f[i];
            Serial.print("class: ");
            Serial.println(i);
            Serial.print(" score: ");
            Serial.println(_value, 6);

            if (_value > maxValue)
            {
                maxValue = _value;
                maxIndex = i;
            }
        }

        uint8_t *maxValueArray;
        maxValueArray = reinterpret_cast<uint8_t*>(&maxValue);
        uint8_t inferenceCharacteristicData[1 + sizeof(float)] = {maxIndex, maxValueArray[0], maxValueArray[1], maxValueArray[2], maxValueArray[3]};

        pInferenceCharacteristic->setValue(inferenceCharacteristicData, sizeof(inferenceCharacteristicData));
        if (ble::isServerConnected) {
            pInferenceCharacteristic->notify();
        }

        lastCaptureTime = currentTime;
    }

    void stop() {
        enabled = false;
        uint8_t pEnabledCharacteristicData[1] = {0};
        pEnabledCharacteristic->setValue(pEnabledCharacteristicData, sizeof(pEnabledCharacteristicData));
    }
} // namespace tfLite