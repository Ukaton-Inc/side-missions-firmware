#pragma once
#ifndef _BLE_HAPTICS_
#define _BLE_HAPTICS_

#include "ble/ble.h"

namespace bleHaptics
{
    extern BLECharacteristic *pVibrationCharacteristic;
    class VibrationCharacteristicCallbacks;

    void setup();
} // namespace bleHaptics

#endif // _BLE_HAPTICS_