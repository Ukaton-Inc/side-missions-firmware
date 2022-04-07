#pragma once
#ifndef _BLE_HAPTICS_
#define _BLE_HAPTICS_

#include "ble/ble.h"

namespace bleHaptics
{
    extern BLECharacteristic *pVibrationCharacteristic;
    class VibrationCharacteristicCallbacks;

    enum class VibrationType : uint8_t
    {
        WAVEFORM = 0,
        SEQUENCE,
        COUNT
    };

    void setup();
    void loop();
} // namespace bleHaptics

#endif // _BLE_HAPTICS_