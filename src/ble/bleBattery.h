#pragma once
#ifndef _BLE_BATTERY_
#define _BLE_BATTERY_

#include "ble.h"

namespace bleBattery
{
    extern BLEService *pService;
    extern BLECharacteristic *pLevelCharacteristic;
    
    void setup();
    void loop();
} // namespace bleBattery

#endif // _BLE_BATTERY_