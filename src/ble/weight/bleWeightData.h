#pragma once
#ifndef _BLE_WEIGHT_DATA_
#define _BLE_WEIGHT_DATA_

#include "ble/ble.h"

namespace bleWeightData
{
    extern BLECharacteristic *pDelayCharacteristic;
    class DelayCharacteristicCallbacks;
    extern BLECharacteristic *pDataCharacteristic;

    void updateDelayCharacteristic();
    void clearDelay();

    void setup();
    void loop();
} // namespace bleWeightData

#endif // _BLE_WEIGHT_DATA_