#pragma once
#ifndef _BATTERY_
#define _BATTERY_

#include <Arduino.h>

namespace battery
{
    extern unsigned long lastUpdateBatteryLevelTime;
    float getLevel();

    void setup();
    void loop();
}

#endif // _BATTERY_