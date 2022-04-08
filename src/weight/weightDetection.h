#pragma once
#ifndef _WEIGHT_DETECTION_
#define _WEIGHT_DETECTION_

#include <Arduino.h>

namespace weightDetection
{
    const std::string filePath = "/weight_detection.tflite";
    void loadModel(bool override = false);
    float guessWeight();
} // namespace weightDetection

#endif // _WEIGHT_DETECTION_