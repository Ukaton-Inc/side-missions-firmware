#pragma once
#ifndef _WEARABLE_DEVICE_
#define _WEARABLE_DEVICE_

#include "ble.h"

namespace wearableDevice
{
    extern uint8_t information[19];
    extern BLECharacteristic *pInformationCharacteristic;
    void setup();
}

#endif // _WEARABLE_DEVICE_