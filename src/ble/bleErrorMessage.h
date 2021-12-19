#pragma once
#ifndef _BLE_ERROR_MESSAGE_
#define _BLE_ERROR_MESSAGE_

#include "ble.h"

namespace bleErrorMessage
{
    extern BLECharacteristic *pErrorMessageCharacteristic;

    void setup();
    void send(const char* message);
} // namespace bleErrorMessage

#endif // _BLE_ERROR_MESSAGE_