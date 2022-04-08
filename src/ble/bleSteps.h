#pragma once
#ifndef _BLE_STEPS_
#define _BLE_STEPS_

#include "ble/ble.h"

namespace bleSteps
{
    extern BLECharacteristic *pIsTrackingCharacteristic;
    class IsTrackingCharacteristicCallbacks;
    extern BLECharacteristic *pMassThresholdCharacteristic;
    class MassThresholdCharacteristicCallbacks;
    extern BLECharacteristic *pDataCharacteristic;
    class DataCharacteristicCallbacks;

    void updateIsTrackingCharacteristic();

    void setup();
    void loop();
} // namespace bleSteps

#endif // _BLE_STEPS_