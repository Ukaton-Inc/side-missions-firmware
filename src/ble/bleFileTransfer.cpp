#include "definitions.h"
#include "bleFileTransfer.h"

#include <FS.h>
#ifdef USE_LittleFS
#define SPIFFS LITTLEFS
#include <LITTLEFS.h>
#else
#include <SPIFFS.h>
#endif

namespace bleFileTransfer
{
    File file;
    std::string filePath = "";

    uint32_t fileSize = 0;
    bool didReadFileSize = false;

    uint32_t bytesTransferred = 0;
    uint8_t fileBuffer[max_file_block_size];

    FileTransferStatus fileTransferStatus = FileTransferStatus::IDLE;
    bool shouldUpdateStatusCharacteristic = false;
    void updateFileTransferStatus(FileTransferStatus status)
    {
        fileTransferStatus = status;
        shouldUpdateStatusCharacteristic = true;
        bytesTransferred = 0;
    }

    void startFileReceive()
    {
        Serial.println("start file receive");
        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }
        file = SPIFFS.open(filePath.c_str(), FILE_WRITE);
        updateFileTransferStatus(FileTransferStatus::RECEIVING_FILE);
    }
    void startFileSend()
    {
        Serial.println("start file send");
        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }
        if (SPIFFS.exists(filePath.c_str())) {
            file = SPIFFS.open(filePath.c_str(), FILE_READ);
            fileSize = file.size();
        }
        else {
            fileSize = 0;
        }

        pFileSizeCharacteristic->setValue(fileSize);
        didReadFileSize = false;

        if (fileSize > 0) {
            updateFileTransferStatus(FileTransferStatus::SENDING_FILE);
        }
    }
    void cancelFileTransfer()
    {
        Serial.println("cancel file transfer");
        if (file)
        {
            file.close();
        }
        SPIFFS.end();
        updateFileTransferStatus(FileTransferStatus::IDLE);
    }

    bool shouldRemoveFile = false;
    void removeFile() {
        Serial.println("will remove file");
        updateFileTransferStatus(FileTransferStatus::REMOVING_FILE);
        shouldRemoveFile = true;
    }

    bool shouldFormatFilesystem = false;
    void formatFilesystem() {
        Serial.println("will format filesystem");
        updateFileTransferStatus(FileTransferStatus::FORMATING_FILESYSTEM);
        shouldFormatFilesystem = true;
    }
    void finishFileTransfer()
    {
        Serial.println("finished file transfer");
        if (file)
        {
            file.close();
        }
        SPIFFS.end();
        updateFileTransferStatus(FileTransferStatus::IDLE);
    }

    BLECharacteristic *pMaxFileSizeCharacteristic;
    BLECharacteristic *pFileSizeCharacteristic;
    BLECharacteristic *pMaxFilePathLengthCharacteristic;
    BLECharacteristic *pFilePathCharacteristic;
    BLECharacteristic *pCommandCharacteristic;
    BLECharacteristic *pStatusCharacteristic;
    BLECharacteristic *pDataCharacteristic;

    class FileSizeCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = (uint8_t *)pCharacteristic->getValue().data();
            uint32_t _fileSize = (((uint32_t)data[3]) << 24) | (((uint32_t)data[2]) << 16) | ((uint32_t)data[1]) << 8 | ((uint32_t)data[0]);
            Serial.printf("received file size: %u bytes\n", fileSize);

            if (_fileSize <= max_file_size)
            {
                fileSize = _fileSize;
                Serial.printf("updated file size to %u bytes\n", fileSize);
            }
            else
            {
                Serial.println("file size is too big");
            }
        }
        void onRead(NimBLECharacteristic* pCharacteristic)
        {
            Serial.println("read file size");
            didReadFileSize = true;
        }
    };

    class FilePathCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto _filePath = pCharacteristic->getValue();
            Serial.printf("received filePath: %s\n", _filePath.c_str());

            if (_filePath.length() <= max_file_path_length)
            {
                filePath = _filePath;
                Serial.printf("updated filePath to %s\n", filePath.c_str());
            }
            else
            {
                Serial.println("filePath is too long");
            }
        }
    };

    class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto command = (Command)pCharacteristic->getValue().data()[0];
            Serial.printf("got file transfer command: %d\n", (uint8_t)command);

            switch (command)
            {
            case Command::START_FILE_RECEIVE:
                startFileReceive();
                break;
            case Command::START_FILE_SEND:
                startFileSend();
                break;
            case Command::CANCEL_FILE_TRANSFER:
                cancelFileTransfer();
                break;
            case Command::REMOVE_FILE:
                removeFile();
                break;
            case Command::FORMAT_FILESYSTEM:
                formatFilesystem();
                break;
            default:
                Serial.printf("Invalid filetransfer command %d", (uint8_t)command);
                break;
            }
        }
    };

    class DataCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto value = pCharacteristic->getValue();
            auto data = (uint8_t *) value.data();
            auto length = value.length();

            Serial.printf("received %u bytes\n", length);

            if (fileTransferStatus == FileTransferStatus::RECEIVING_FILE)
            {
                if (file)
                {
                    file.write(data, length);
                    bytesTransferred += length;
                    int bytesLeft = fileSize - bytesTransferred;
                    Serial.printf("file size is now %u bytes, so we have %d bytes left\n", bytesTransferred, bytesLeft);
                    if (bytesLeft == 0)
                    {
                        Serial.println("Finished writing file!");
                        finishFileTransfer();
                    }
                }
                else
                {
                    Serial.println("file not found");
                }
            }
            else
            {
                Serial.println("receiving file blocks when not expecting any");
            }
        }
    };

    void sendFileData()
    {
        auto bytesLeftToRead = fileSize - bytesTransferred;
        auto bytesToSend = (bytesLeftToRead > max_file_block_size) ? max_file_block_size : bytesLeftToRead;
        file.read(fileBuffer, bytesToSend);
        bytesTransferred += bytesToSend;

        Serial.printf("sent %u bytes of %u\n", bytesTransferred, fileSize);

        pDataCharacteristic->setValue(fileBuffer, bytesToSend);
        if (pDataCharacteristic->getSubscribedCount() > 0)
        {
            pDataCharacteristic->notify();
        }

        if (bytesTransferred == fileSize)
        {
            finishFileTransfer();
        }
    }

    void _removeFile() {
        Serial.println("removing file...");

        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }

        if (SPIFFS.exists(filePath.c_str())) {
            SPIFFS.remove(filePath.c_str());
            Serial.println("removed file");
        }
        else {
            Serial.println("file doesn't exist");
        }
        SPIFFS.end();
        
        updateFileTransferStatus(FileTransferStatus::IDLE);
    }
    void _formatFilesystem() {
        Serial.println("formatting filesystem...");

        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }
        SPIFFS.format();
        SPIFFS.end();
        updateFileTransferStatus(FileTransferStatus::IDLE);
        Serial.println("formatted filesystem");
    }

    void setup()
    {
        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }
        SPIFFS.end();

        pMaxFileSizeCharacteristic = ble::createCharacteristic(GENERATE_UUID("a000"), NIMBLE_PROPERTY::READ, "Max File Size");
        pFileSizeCharacteristic = ble::createCharacteristic(GENERATE_UUID("a001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY, "File Size");
        pMaxFilePathLengthCharacteristic = ble::createCharacteristic(GENERATE_UUID("a002"), NIMBLE_PROPERTY::READ, "Max FilePath Length");
        pFilePathCharacteristic = ble::createCharacteristic(GENERATE_UUID("a003"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "File Name");
        pCommandCharacteristic = ble::createCharacteristic(GENERATE_UUID("a004"), NIMBLE_PROPERTY::WRITE, "File Command");
        pStatusCharacteristic = ble::createCharacteristic(GENERATE_UUID("a005"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "File Status");
        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("a006"), NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY, "File Data");

        pFileSizeCharacteristic->setCallbacks(new FileSizeCharacteristicCallbacks());
        pFilePathCharacteristic->setCallbacks(new FilePathCharacteristicCallbacks());
        pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());
        pDataCharacteristic->setCallbacks(new DataCharacteristicCallbacks());

        pMaxFileSizeCharacteristic->setValue(max_file_size);
        pFileSizeCharacteristic->setValue(fileSize);
        pMaxFilePathLengthCharacteristic->setValue(max_file_path_length);
        pFilePathCharacteristic->setValue(filePath);
        pStatusCharacteristic->setValue(fileTransferStatus);
    }

    unsigned long currentTime;
    unsigned long lastFileDataSendTime;
    void loop()
    {
        currentTime = millis();

        if (shouldUpdateStatusCharacteristic)
        {
            pStatusCharacteristic->setValue(fileTransferStatus);
            if (pStatusCharacteristic->getSubscribedCount() > 0)
            {
                pStatusCharacteristic->notify();
            }
            shouldUpdateStatusCharacteristic = false;
        }

        if (didReadFileSize && fileTransferStatus == FileTransferStatus::SENDING_FILE && currentTime >= lastFileDataSendTime + file_block_delay_ms)
        {
            sendFileData();
            lastFileDataSendTime = currentTime - (currentTime % file_block_delay_ms);
        }

        if (shouldRemoveFile) {
            _removeFile();
            shouldRemoveFile = false;
        }

        if (shouldFormatFilesystem) {
            _formatFilesystem();
            shouldFormatFilesystem = false;
        }
    }
} // namespace fileTransfer