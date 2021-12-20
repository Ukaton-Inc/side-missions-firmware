#pragma once
#ifndef _BLE_MOTION_CALIBRATION_
#define _BLE_MOTION_CALIBRATION_

#include "ble.h"

namespace bleMotionCalibration
{
    extern BLECharacteristic *pCharacteristic;

    void setup();
    void loop();
} // namespace bleMotionCalibration

#endif // _BLE_MOTION_CALIBRATION_