#pragma once
#ifndef _BATTERY_
#define _BATTERY_

#include "ble.h"

namespace battery
{
    extern const uint16_t battery_level_delay_ms;
    extern uint8_t batteryLevel;

    extern BLEService *pBatteryService;
    
    extern BLECharacteristic *pBatteryLevelCharacteristic;
    extern uint8_t batteryLevelCharacteristicValue[1];

    uint8_t getBatteryLevel();
    void updateBatteryLevel();

    extern unsigned long lastTime;
    extern unsigned long currentTime;

    void setup();
    void loop();
}

#endif // _BATTERY_