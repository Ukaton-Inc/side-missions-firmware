#pragma once
#ifndef _BLE_FILE_TRANSFER_
#define _BLE_FILE_TRANSFER_

#include <lwipopts.h>
#include "ble.h"

#include "SPIFFS.h"

namespace bleFileTransfer
{
    typedef enum: uint8_t {
        WEIGHT_DETECTION_MODEL = 0,
        GENERIC_TF_LITE_MODEL_FILE,
        COUNT
    } FileType;
    extern FileType fileTransferType;

    constexpr uint32_t max_file_size = (2 * 1024 * 1024); // 2 mb
    constexpr uint16_t max_file_block_size = 512;
    
    typedef enum: uint8_t {
        START_FILE_TRANSFER = 0,
        CANCEL_FILE_TRANSFER
    } Command;

    extern BLECharacteristic *pMaxFileSizeCharacteristic;
    extern BLECharacteristic *pFileTypeCharacteristic;
    extern BLECharacteristic *pCommandCharacteristic;
    extern BLECharacteristic *pDataCharacteristic;

    class FileTypeCharacteristicCallbacks;
    class CommandCharacteristicCallbacks;
    class DataCharacteristicCallbacks;

    void setup();
} // namespace bleFileTransfer

#endif // _BLE_FILE_TRANSFER_