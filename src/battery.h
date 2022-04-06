#pragma once
#ifndef _BATTERY_
#define _BATTERY_

#include <Arduino.h>

namespace battery
{
    extern float voltage;
    extern float soc;

    extern unsigned long lastUpdateBatteryLevelTime;

    void setup();
    void loop();
}

#endif // _BATTERY_