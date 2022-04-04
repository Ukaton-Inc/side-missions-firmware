#pragma once
#ifndef _TFLITE_TEMPLATE_
#define _TFLITE_TEMPLATE_

#include <Arduino.h>

namespace tflite_template
{
    const std::string filePath = "/tflite_file.tflite";
    void loadModel(bool override = false); // call when tflite file is received over ble or websockets
    void makeInference();
} // namespace tflite_template

#endif // _TFLITE_TEMPLATE_