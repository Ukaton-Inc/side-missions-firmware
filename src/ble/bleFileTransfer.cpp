#include "definitions.h"
#include "bleFileTransfer.h"

#include <Update.h>
#include "FS.h"
#include "SPIFFS.h"

namespace bleFileTransfer
{
    FileType fileTransferType;

    BLECharacteristic *pMaxFileSizeCharacteristic;
    BLECharacteristic *pFileTypeCharacteristic;
    BLECharacteristic *pCommandCharacteristic;
    BLECharacteristic *pDataCharacteristic;

    class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto commandData = pCharacteristic->getValue().data();
            auto command = (Command) commandData[0];

            Serial.print("set file transfer command: ");
            Serial.println((uint8_t) command);

            switch (command)
            {
            case START_FILE_TRANSFER:
                Serial.println("received start_file_transfer command...");
                // FILL
                break;
            case CANCEL_FILE_TRANSFER:
                Serial.println("received cancel_file_transfer command...");
                // FILL
                break;
            default:
                Serial.printf("Invalid filetransfer command %d", command);
                break;
            }
        }
    };

    class FileTypeCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *data = (uint8_t *)pCharacteristic->getValue().data();
            unsigned char rawFileTransferType = data[0];
            Serial.print("set file transfer type: ");
            Serial.println(rawFileTransferType);
            fileTransferType = (FileType)rawFileTransferType;
        }
    };

    class DataCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *data = (uint8_t *)pCharacteristic->getValue().data();
        }
    };

    void setup()
    {
        pMaxFileSizeCharacteristic = ble::createCharacteristic(GENERATE_UUID("a000"), NIMBLE_PROPERTY::READ, "Max File Size");
        pFileTypeCharacteristic = ble::createCharacteristic(GENERATE_UUID("a001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "File Type");
        pCommandCharacteristic = ble::createCharacteristic(GENERATE_UUID("a002"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "File Command");
        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("a003"), NIMBLE_PROPERTY::WRITE, "File Data");

        pFileTypeCharacteristic->setCallbacks(new FileTypeCharacteristicCallbacks());
        pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
        pDataCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());

        uint32_t maxFileSizeCharacteristicData[1] = {max_file_size};
        pMaxFileSizeCharacteristic->setValue((uint8_t *)maxFileSizeCharacteristicData, sizeof(max_file_size));
    }
} // namespace fileTransfer