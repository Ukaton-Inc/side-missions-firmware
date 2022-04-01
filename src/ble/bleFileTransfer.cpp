#include "definitions.h"
#include "bleFileTransfer.h"

#include <Update.h>
#include "FS.h"
#include "SPIFFS.h"

namespace bleFileTransfer
{
    FileType fileTransferType = FileType::NO_TYPE;
    FileTransferStatus fileTransferStatus = FileTransferStatus::IDLE;

    uint32_t fileSize = 0;

    BLECharacteristic *pMaxFileSizeCharacteristic;
    BLECharacteristic *pFileTypeCharacteristic;
    BLECharacteristic *pFileSizeCharacteristic;
    BLECharacteristic *pCommandCharacteristic;
    BLECharacteristic *pStatusCharacteristic;
    BLECharacteristic *pDataCharacteristic;

    class FileTypeCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto rawFileTransferType = (uint8_t) pCharacteristic->getValue().data()[0];
            Serial.printf("got file transfer type: %d\n", rawFileTransferType);

            if (rawFileTransferType < FileType::COUNT) {
                fileTransferType = (FileType)rawFileTransferType;
            }
            else {
                Serial.println("invalid file transfer type");
            }
        }
    };

    class FileSizeCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = (uint8_t *) pCharacteristic->getValue().data();
            uint32_t _fileSize = (((uint32_t)data[3]) << 24) | (((uint32_t)data[2]) << 16) | ((uint32_t)data[1]) << 8 | ((uint32_t)data[0]);
            Serial.printf("received file size: %u bytes\n", fileSize);

            if (_fileSize <= max_file_size) {
                fileSize = _fileSize;
                Serial.printf("updated file size to %u bytes\n", fileSize);
            }
            else {
                Serial.println("file size too big");
            }
        }
    };

    class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto command = (Command) pCharacteristic->getValue().data()[0];
            Serial.printf("got file transfer command: %d\n", (uint8_t) command);

            switch (command)
            {
            case Command::START_FILE_SEND:
                Serial.println("start file send");
                fileTransferStatus = FileTransferStatus::SENDING_FILE;
                break;
            case Command::START_FILE_RECEIVE:
                Serial.println("start file receive");
                fileTransferStatus = FileTransferStatus::RECEIVING_FILE;
                break;
            case Command::CANCEL_FILE_TRANSFER:
                Serial.println("cancel file transfer");
                fileTransferStatus = FileTransferStatus::IDLE;
                break;
            default:
                Serial.printf("Invalid filetransfer command %d", (uint8_t) command);
                fileTransferStatus = FileTransferStatus::IDLE;
                break;
            }
        }
    };

    class DataCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = (uint8_t *) pCharacteristic->getValue().data();
            Serial.println("got data");

            if (fileTransferStatus == FileTransferStatus::RECEIVING_FILE) {
                // FILL - append to file
            }
            else {
                Serial.println("receiving file blocks when not expecting any");
            }
        }
    };

    void setup()
    {
        SPIFFS.begin();
        
        pMaxFileSizeCharacteristic = ble::createCharacteristic(GENERATE_UUID("a000"), NIMBLE_PROPERTY::READ, "Max File Size");
        pFileTypeCharacteristic = ble::createCharacteristic(GENERATE_UUID("a001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "File Type");
        pFileSizeCharacteristic = ble::createCharacteristic(GENERATE_UUID("a002"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "File Size");
        pCommandCharacteristic = ble::createCharacteristic(GENERATE_UUID("a003"), NIMBLE_PROPERTY::WRITE, "File Command");
        pStatusCharacteristic = ble::createCharacteristic(GENERATE_UUID("a004"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "File Status");
        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("a005"), NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY, "File Data");

        pFileTypeCharacteristic->setCallbacks(new FileTypeCharacteristicCallbacks());
        pFileSizeCharacteristic->setCallbacks(new FileSizeCharacteristicCallbacks());
        pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
        pDataCharacteristic->setCallbacks(new DataCharacteristicCallbacks());

        pMaxFileSizeCharacteristic->setValue(max_file_size);
        pFileTypeCharacteristic->setValue(fileTransferType);
        pFileSizeCharacteristic->setValue(fileSize);
        pStatusCharacteristic->setValue(fileTransferStatus);
    }
} // namespace fileTransfer