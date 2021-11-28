#pragma once
#ifndef _TF_LITE_
#define _TF_LITE_

#undef DEFAULT
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "ble.h"
#include "definitions.h"
#include "motion.h"
namespace tfLite
{
    extern bool isModelLoaded;
    extern uint8_t interpreterBuffer[sizeof(tflite::MicroInterpreter)];
    constexpr int tensorArenaSize = 8 * 1024;
    alignas(16) extern byte tensorArena[tensorArenaSize];
    
    extern tflite::MicroErrorReporter tflErrorReporter;
    extern tflite::AllOpsResolver tflOpsResolver;

    extern const tflite::Model *tflModel;
    extern tflite::MicroInterpreter *tflInterpreter;
    extern TfLiteTensor *tflInputTensor;
    extern TfLiteTensor *tflOutputTensor;

    typedef enum: uint8_t
    {
        LINEAR_ACCELERATION_THRESHOLD = 0,
        ROTATION_RATE_THRESHOLD,
        NUMBER_OF_THRESHOLDS = 2
    } Thresholds;

    typedef enum: uint8_t
    {
        ACCELERATION = 0,
        GRAVITY,
        LINEAR_ACCELERATION,
        ROTATION_RATE,
        MAGNETOMETER,
        QUATERNION,
        NUMBER_OF_DATA_TYPES = 6
    } DataTypes;

    typedef enum: uint8_t
    {
        ACCELERATION_BITMASK_INDEX = 0,
        GRAVITY_BITMASK_INDEX,
        LINEAR_ACCELERATION_BITMASK_INDEX,
        ROTATION_RATE_BITMASK_INDEX,
        MAGNETOMETER_BITMASK_INDEX,
        QUATERNION_BITMASK_INDEX
    } DataTypeBitmaskIndices;

    typedef enum: uint8_t
    {
        CLASSIFICATION = 0,
        REGRESSION,
        NUMBER_OF_MODEL_TYPES = 2,
    } ModelTypes;

    extern bool enabled;
    extern uint8_t modelType;
    extern uint8_t numberOfClasses;
    extern uint8_t dataTypesBitmask;
    extern uint16_t sampleRate;
    extern uint16_t numberOfSamples;
    extern float thresholds[NUMBER_OF_THRESHOLDS];
    extern uint16_t captureDelay;

    extern bool isCapturingSamples;
    extern uint8_t sampleDataSize;
    extern uint16_t numberOfSamplesRead;
    extern unsigned long currentTime;
    extern unsigned long lastUpdateLoopTime;
    extern unsigned long lastCaptureTime;
    
    extern BLECharacteristic *pHasModelCharacteristic;
    extern BLECharacteristic *pEnabledCharacteristic;
    extern BLECharacteristic *pModelTypeCharacteristic;
    extern BLECharacteristic *pNumberOfClassesCharacteristic;
    extern BLECharacteristic *pDataTypesCharacteristic;
    extern BLECharacteristic *pSampleRateCharacteristic;
    extern BLECharacteristic *pNumberOfSamplesCharacteristic;
    extern BLECharacteristic *pThresholdCharacteristic;
    extern BLECharacteristic *pCaptureDelayCharacteristic;
    extern BLECharacteristic *pInferenceCharacteristic;
    extern BLECharacteristic *pMakeInferenceCharacteristic;

    class EnabledCharacteristicCallbacks;
    class ModelTypeCharacteristicCallbacks;
    class NumberOfClassesCharacteristicCallbacks;
    class DataTypesCharacteristicCallbacks;
    class SampleRateCharacteristicCallbacks;
    class NumberOfSamplesCharacteristicCallbacks;
    class ThresholdCharacteristicCallbacks;
    class CaptureDelayCharacteristicCallbacks;
    class MakeInferenceCharacteristicCallbacks;

    void setup();
    void loadModel(unsigned char model[]);
    void loop();
    void updateLoop();
    void update();
    void startCapturingSamples();
    void makeInference();
    void stop();
} // namespace tfLite

#endif // _TF_LITE_