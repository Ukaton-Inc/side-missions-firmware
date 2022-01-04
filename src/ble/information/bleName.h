#pragma once
#ifndef _BLE_NAME_
#define _BLE_NAME_

#include "ble/ble.h"

namespace bleName
{
    extern BLECharacteristic *pCharacteristic;
    class CharacteristicCallbacks;

    const std::string *getName();

    void setup();
} // namespace bleName

#endif // _BLE_NAME_