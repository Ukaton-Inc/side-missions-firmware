#include "gestures.h"
#include "gestures_model.h"
#include "sensors.h"

namespace gestures
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
    uint8_t dataTypesBitmask = 0b1100; // Linear Acceleration + Rotation Rate
    uint16_t sampleRate = 20;
    uint16_t numberOfSamples = 26;
    float thresholds[NUMBER_OF_THRESHOLDS] = {4.6, 0};
    uint16_t captureDelay = 1000;

    bool isCapturingSamples = false;
    uint8_t sampleDataSize;
    uint16_t numberOfSamplesRead = 0;
    unsigned long currentTime;
    unsigned long lastUpdateLoopTime = 0;
    unsigned long lastCaptureTime = 0;

    uint8_t information[NUMBER_OF_GESTURES * 3] = {DOUBLE_TAP, 1, 0, HEAD_NOD, 1, 0, HEAD_SHAKE, 1, 0, INPUT_GESTURE, 1, 0, AFFIRMATIVE, 1, 0, NEGATIVE, 1, 0};
    uint8_t configuration[NUMBER_OF_GESTURES * 2] = {DOUBLE_TAP, 0, HEAD_NOD, 0, HEAD_SHAKE, 0, INPUT_GESTURE, 0, AFFIRMATIVE, 0, NEGATIVE, 0};

    BLECharacteristic *pInformationCharacteristic;
    BLECharacteristic *pConfigurationCharacteristic;
    BLECharacteristic *pDataCharacteristic;

    class GestureConfigurationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *data = (uint8_t *)pCharacteristic->getValue().data();
            uint8_t length = pCharacteristic->getDataLength();
            for (uint8_t index = 0; index < NUMBER_OF_GESTURES && (index + 1) * 2 <= length; index++)
            {
                uint8_t gestureId = data[index * 2];
                int8_t gestureIndex = -1;
                switch (gestureId)
                {
                case DOUBLE_TAP:
                    gestureIndex = 0;
                    break;
                case HEAD_NOD:
                    gestureIndex = 1;
                    break;
                case HEAD_SHAKE:
                    gestureIndex = 2;
                    break;
                case INPUT_GESTURE:
                    gestureIndex = 3;
                    break;
                case AFFIRMATIVE:
                    gestureIndex = 4;
                    break;
                case NEGATIVE:
                    gestureIndex = 5;
                    break;
                }
                if (gestureIndex >= 0)
                {
                    bool isGestureEnabled = data[index * 2 + 1] == 1;
                    configuration[gestureIndex * 2 + 1] = isGestureEnabled;
                }
            }

            enabled = false;
            for (uint8_t gestureIndex = 0; gestureIndex < NUMBER_OF_GESTURES && !enabled; gestureIndex++)
            {
                bool isGestureEnabled = configuration[gestureIndex * 2 + 1];
                enabled = enabled || isGestureEnabled;
            }

            pCharacteristic->setValue(configuration, sizeof(configuration));
            pCharacteristic->notify();
        }
    };

    void setup()
    {
        pInformationCharacteristic = ble::createCharacteristic("A0384F52-F95A-4BCD-B898-7B9CEEC92DAD", NIMBLE_PROPERTY::READ, "Gesture information");
        pInformationCharacteristic->setValue(information, sizeof(information));

        pConfigurationCharacteristic = ble::createCharacteristic("21E550AF-F780-477B-9334-1F983296F1D7", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY, "Gesture configuration");
        pConfigurationCharacteristic->setValue(configuration, sizeof(configuration));
        pConfigurationCharacteristic->setCallbacks(new GestureConfigurationCharacteristicCallbacks());

        pDataCharacteristic = ble::createCharacteristic("9014DD4E-79BA-4802-A275-894D3B85AC74", NIMBLE_PROPERTY::NOTIFY, "Gesture data");

        if (gestures_model_length > 0)
        {
            loadModel(gestures_model);
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
            bool reachedThresholds = false;

            for (uint8_t i = 0; i < NUMBER_OF_THRESHOLDS && !reachedThresholds; i++)
            {
                if (thresholds[i] > 0)
                {
                    const float threshold = thresholds[i];
                    if (i == LINEAR_ACCELERATION_THRESHOLD)
                    {
                        imu::Vector<3> linearAccelerationVector = sensors::bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
                        if (linearAccelerationVector.magnitude() > threshold)
                        {
                            reachedThresholds = true;
                        }
                    }
                    else if (i == ROTATION_RATE_THRESHOLD)
                    {
                        imu::Vector<3> rotationRateVector = sensors::bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
                        if (rotationRateVector.magnitude() > threshold)
                        {
                            reachedThresholds = true;
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

                    switch (i)
                    {
                    case ACCELERATION_BITMASK_INDEX:
                        sensors::bno.getRawVectorData(Adafruit_BNO055::VECTOR_ACCELEROMETER, buffer);
                        size = 3;
                        range = sensors::ACCELEROMETER_RANGE;
                        break;
                    case GRAVITY_BITMASK_INDEX:
                        sensors::bno.getRawVectorData(Adafruit_BNO055::VECTOR_GRAVITY, buffer);
                        size = 3;
                        range = sensors::ACCELEROMETER_RANGE;
                        break;
                    case LINEAR_ACCELERATION_BITMASK_INDEX:
                        sensors::bno.getRawVectorData(Adafruit_BNO055::VECTOR_LINEARACCEL, buffer);
                        size = 3;
                        range = sensors::ACCELEROMETER_RANGE;
                        break;
                    case ROTATION_RATE_BITMASK_INDEX:
                        sensors::bno.getRawVectorData(Adafruit_BNO055::VECTOR_GYROSCOPE, buffer);
                        size = 3;
                        range = sensors::GYROSCOPE_RANGE;
                        break;
                    case MAGNETOMETER_BITMASK_INDEX:
                        sensors::bno.getRawVectorData(Adafruit_BNO055::VECTOR_MAGNETOMETER, buffer);
                        size = 3;
                        range = sensors::MAGNETOMETER_RANGE;
                        break;
                    case QUATERNION_BITMASK_INDEX:
                        sensors::bno.getRawQuatData(buffer);
                        size = 4;
                        range = 1;
                        break;
                    }

                    for (uint8_t i = 0; i < size; i++)
                    {
                        tflInputTensor->data.f[(numberOfSamplesRead * sampleDataSize) + offset + i] = (float)buffer[i] / (float)range;
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
    void startCapturingSamples()
    {
        if (isCapturingSamples)
        {
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
        for (uint8_t i = 0; i < NUMBER_OF_UNIQUE_GESTURES+1; i++)
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

        if (ble::isServerConnected)
        {
            uint8_t gestureIndex = maxIndex;
            if (gestureIndex < NUMBER_OF_UNIQUE_GESTURES && configuration[gestureIndex * 2 + 1] == 1)
            {
                int gestureId = -1;
                int gestureId2 = -1;
                switch (gestureIndex)
                {
                case 0:
                    gestureId = DOUBLE_TAP;
                    gestureId2 = INPUT_GESTURE;
                    break;
                case 1:
                    gestureId = HEAD_NOD;
                    gestureId2 = AFFIRMATIVE;
                    break;
                case 2:
                    gestureId = HEAD_SHAKE;
                    gestureId2 = NEGATIVE;
                    break;
                }

                if (gestureId >= 0) {
                    uint8_t gestureData[3] = {0, highByte((uint16_t)currentTime), lowByte((uint16_t)currentTime)};

                    if (configuration[gestureIndex*2+1] == 1) {
                        gestureData[0] = gestureId;
                        pDataCharacteristic->setValue(gestureData, sizeof(gestureData));
                        pDataCharacteristic->notify();
                    }

                    if (configuration[(gestureIndex+3)*2+1] == 1) {                        
                        gestureData[0] = gestureId2;
                        pDataCharacteristic->setValue(gestureData, sizeof(gestureData));
                        pDataCharacteristic->notify();
                    }
                }
            }
        }
        lastCaptureTime = currentTime;
    }

    void stop()
    {
        enabled = false;
        for (uint8_t gestureIndex = 0; gestureIndex < NUMBER_OF_GESTURES; gestureIndex++)
        {
            configuration[gestureIndex * 2 + 1] = 0;
        }
        pConfigurationCharacteristic->setValue(configuration, sizeof(configuration));
    }
} // namespace gestures