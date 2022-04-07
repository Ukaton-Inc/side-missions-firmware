#pragma once
#ifndef _BLE_HAPTICS_
#define _BLE_HAPTICS_

#include "ble/ble.h"

namespace bleHaptics
{
    extern BLECharacteristic *pWaveformCharacteristic;
    class WaveformCharacteristicCallbacks;

    void setup();
    void loop();
} // namespace bleHaptics

#endif // _BLE_HAPTICS_