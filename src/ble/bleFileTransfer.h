#pragma once
#ifndef _BLE_FILE_TRANSFER_
#define _BLE_FILE_TRANSFER_

#include <lwipopts.h>
#include "ble.h"

namespace bleFileTransfer
{
    constexpr uint16_t file_maximum_byte_count = (25 * 1024); // 25 kb
    constexpr uint8_t file_block_byte_count = 128;

    extern uint8_t *file_buffers[2];

    typedef enum: uint8_t {
        FIRMWARE = 0,
        WEIGHT_DETECTION_MODEL,
        GENERIC_TF_LITE_MODEL_FILE,
        COUNT
    } FileTransferType;
    extern FileTransferType fileTransferType;
    
    typedef enum: uint8_t {
        START_FILE_TRANSFER = 0,
        CANCEL_FILE_TRANSFER
    } Command;

    typedef enum: uint8_t {
        SUCCESS_STATUS = 0,
        ERROR_STATUS,
        IN_PROGRESS_STATUS
    } Status;

    extern uint8_t *in_progress_file_buffer;
    extern int32_t in_progress_bytes_received;
    extern int32_t in_progress_bytes_expected;
    extern uint32_t in_progress_checksum;

    extern int finished_file_buffer_index;
    extern uint8_t *finished_file_buffer;
    extern int32_t finished_file_buffer_byte_count;

    extern BLECharacteristic *pFileBlockCharacteristic;
    extern BLECharacteristic *pFileLengthCharacteristic;
    extern BLECharacteristic *pFileMaximumLengthCharacteristic;
    extern BLECharacteristic *pFileTransferTypeCharacteristic;
    extern BLECharacteristic *pChecksumCharacteristic;
    extern BLECharacteristic *pCommandCharacteristic;
    extern BLECharacteristic *pTransferStatusCharacteristic;
    extern BLECharacteristic *pErrorMessageCharacteristic;

    class FileBlockCharacteristicCallbacks;
    class FileTransferTypeCharacteristicCallbacks;
    class CommandCharacteristicCallbacks;

    constexpr int32_t error_message_byte_count = 128;

    void setup();

    bool isTransfering();
    void startFileTransfer();
    void cancelFileTransfer();
    void onFileTransferComplete();
    void onFileReceived(uint8_t *file_data, int file_length);

    void notifySuccess();
    void notifyInProgress();
    void notifyError(const String &errorMessage);

    uint32_t crc32_for_byte(uint32_t r);
    uint32_t crc32(const uint8_t *data, size_t data_length);
} // namespace bleFileTransfer

#endif // _BLE_FILE_TRANSFER_