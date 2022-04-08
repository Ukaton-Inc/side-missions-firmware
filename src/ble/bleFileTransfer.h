#pragma once
#ifndef _BLE_FILE_TRANSFER_
#define _BLE_FILE_TRANSFER_

#include <lwipopts.h>
#include "ble.h"

namespace bleFileTransfer
{
    constexpr uint32_t max_file_size = (2 * 1024 * 1024); // 2 mb
    constexpr uint16_t max_file_block_size = 252;
    constexpr uint8_t max_file_path_length = 31; // /filename.tflite
    constexpr uint8_t file_block_delay_ms = 20;

    typedef enum: uint8_t {
        START_FILE_RECEIVE = 0,
        START_FILE_SEND,
        CANCEL_FILE_TRANSFER,
        REMOVE_FILE,
        FORMAT_FILESYSTEM
    } Command;

    typedef enum: uint8_t {
        IDLE = 0,
        RECEIVING_FILE,
        SENDING_FILE,
        REMOVING_FILE,
        FORMATING_FILESYSTEM
    } FileTransferStatus;
    extern FileTransferStatus fileTransferStatus;

    extern BLECharacteristic *pMaxFileSizeCharacteristic;
    extern BLECharacteristic *pFileSizeCharacteristic;
    extern BLECharacteristic *pMaxFileNameLengthCharacteristic;
    extern BLECharacteristic *pFilePathCharacteristic;
    extern BLECharacteristic *pCommandCharacteristic;
    extern BLECharacteristic *pStatusCharacteristic;
    extern BLECharacteristic *pDataCharacteristic;

    class FileSizeCharacteristicCallbacks;
    class FilePathCharacteristicCallbacks;
    class CommandCharacteristicCallbacks;
    class DataCharacteristicCallbacks;

    void setup();
    void loop();
} // namespace bleFileTransfer

#endif // _BLE_FILE_TRANSFER_