#pragma once
#ifndef _WEIGHT_
#define _WEIGHT_

#undef DEFAULT
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

#include "ble.h"

namespace weight {
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

    extern BLECharacteristic *pInferenceCharacteristic;
    extern BLECharacteristic *pMakeInferenceCharacteristic;

    class MakeInferenceCharacteristicCallbacks;

    extern unsigned long currentTime;

    extern unsigned char* weight_model;
    extern unsigned int weight_model_length;

    void setup();
    void loadModel(unsigned char model[]);
    void loop();
    void makeInference();
} // namespace weight

#endif // _WEIGHT_