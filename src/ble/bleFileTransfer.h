#pragma once
#ifndef _BLE_FILE_TRANSFER_
#define _BLE_FILE_TRANSFER_

#include <lwipopts.h>
#include "ble.h"

#include "SPIFFS.h"

namespace bleFileTransfer
{
    constexpr uint32_t max_file_size = (2 * 1024 * 1024); // 2 mb
    constexpr uint16_t max_file_block_size = 512;

    typedef enum: uint8_t {
        NO_TYPE = 0,
        WEIGHT_DETECTION_MODEL,
        GENERIC_TF_LITE_MODEL_FILE,
        COUNT
    } FileType;
    extern FileType fileTransferType;

    typedef enum: uint8_t {
        START_FILE_RECEIVE = 0,
        START_FILE_SEND,
        CANCEL_FILE_TRANSFER
    } Command;

    typedef enum: uint8_t {
        IDLE = 0,
        RECEIVING_FILE,
        SENDING_FILE
    } FileTransferStatus;
    extern FileTransferStatus fileTransferStatus;

    extern BLECharacteristic *pMaxFileSizeCharacteristic;
    extern BLECharacteristic *pFileTypeCharacteristic;
    extern BLECharacteristic *pFileSizeCharacteristic;
    extern BLECharacteristic *pCommandCharacteristic;
    extern BLECharacteristic *pStatusCharacteristic;
    extern BLECharacteristic *pDataCharacteristic;

    class FileTypeCharacteristicCallbacks;
    class FileSizeCharacteristicCallbacks;
    class CommandCharacteristicCallbacks;
    class DataCharacteristicCallbacks;

    void setup();
} // namespace bleFileTransfer

#endif // _BLE_FILE_TRANSFER_