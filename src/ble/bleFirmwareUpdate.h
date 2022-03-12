#pragma once
#ifndef _BLE_FIRMWARE_UPDATE
#define _BLE_FIRMWARE_UPDATE

#include <lwipopts.h>
#include "ble.h"

namespace bleFirmwareUpdate
{
    constexpr uint32_t max_size = (2 * 1024 * 1024); // 2mb
    constexpr uint16_t block_byte_count = 512;

    extern BLECharacteristic *pVersionCharacteristic;
    extern BLECharacteristic *pMaxSizeCharacteristic;
    extern BLECharacteristic *pDataCharacteristic;

    class SizeCharacteristicCallbacks;
    class DataCharacteristicCallbacks;

    void setup();
} // namespace bleFirmwareUpdate

#endif // _BLE_FIRMWARE_UPDATE