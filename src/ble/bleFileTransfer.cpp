#include "definitions.h"
#include "bleFileTransfer.h"

#include <Update.h>
#include "FS.h"
#include "SPIFFS.h"

namespace bleFileTransfer
{
    FileType fileTransferType = FileType::NO_TYPE;
    FileTransferStatus fileTransferStatus = FileTransferStatus::IDLE;

    BLECharacteristic *pMaxFileSizeCharacteristic;
    BLECharacteristic *pFileTypeCharacteristic;
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

    class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto command = (Command) pCharacteristic->getValue().data()[0];
            Serial.printf("got file transfer command: %d\n", (uint8_t) command);

            // FILL - update fileTransferStatus

            switch (command)
            {
            case START_FILE_SEND:
                Serial.println("start file send");
                // FILL
                break;
            case START_FILE_RECEIVE:
                Serial.println("start file receive");
                // FILL
                break;
            case CANCEL_FILE_TRANSFER:
                Serial.println("cancel file transfer");
                // FILL
                break;
            default:
                Serial.printf("Invalid filetransfer command %d", (uint8_t) command);
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
        }
    };

    void setup()
    {
        pMaxFileSizeCharacteristic = ble::createCharacteristic(GENERATE_UUID("a000"), NIMBLE_PROPERTY::READ, "Max File Size");
        pFileTypeCharacteristic = ble::createCharacteristic(GENERATE_UUID("a001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "File Type");
        pCommandCharacteristic = ble::createCharacteristic(GENERATE_UUID("a002"), NIMBLE_PROPERTY::WRITE, "File Command");
        pStatusCharacteristic = ble::createCharacteristic(GENERATE_UUID("a003"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "File Status");
        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("a004"), NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY, "File Data");

        pFileTypeCharacteristic->setCallbacks(new FileTypeCharacteristicCallbacks());
        pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
        pDataCharacteristic->setCallbacks(new DataCharacteristicCallbacks());

        pFileTypeCharacteristic->setValue((uint8_t) fileTransferType);
        pStatusCharacteristic->setValue((uint8_t) fileTransferStatus);
        uint32_t maxFileSizeCharacteristicData[1] = {max_file_size};
        pMaxFileSizeCharacteristic->setValue((uint8_t *)maxFileSizeCharacteristicData, sizeof(max_file_size));
    }
} // namespace fileTransfer