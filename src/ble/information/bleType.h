#pragma once
#ifndef _BLE_TYPE_
#define _BLE_TYPE_

#include "ble/ble.h"

namespace bleType
{
    extern BLECharacteristic *pCharacteristic;
    class CharacteristicCallbacks;

    void setup();
} // namespace bleType

#endif // _BLE_TYPE_