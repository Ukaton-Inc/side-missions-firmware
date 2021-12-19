#pragma once
#ifndef _BLE_DEBUG_
#define _BLE_DEBUG_

#include "ble.h"
#include "debug.h"

namespace bleDebug
{
    extern BLECharacteristic *pCharacteristic;
    class CharacteristicCallbacks;

    bool getEnabled();

    void setup();
} // namespace bleDebug

#endif // _BLE_DEBUG_