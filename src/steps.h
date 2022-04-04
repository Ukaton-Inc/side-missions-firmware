#pragma once
#ifndef _STEPS_
#define _STEPS_

#include <Arduino.h>

namespace steps
{
    bool isTracking();
    void startTracking();
    void stopTracking();

    float getMassThreshold();
    void setMassThreshold(float threshold);

    uint32_t getSteps();
    void setSteps(uint32_t _steps);
    void resetSteps();

    extern unsigned long lastTimeStepsWereUpdated;

    void setup();
    void loop();
} // namespace steps

#endif // _STEPS_