#include "fileTransfer.h"

namespace fileTransfer
{
    FileTransferType fileTransferType = TF_LITE_MODEL_FILE;

    uint8_t *file_buffers[2];

    uint8_t *in_progress_file_buffer = nullptr;
    int32_t in_progress_bytes_received = 0;
    int32_t in_progress_bytes_expected = 0;
    uint32_t in_progress_checksum = 0;

    int finished_file_buffer_index = -1;
    uint8_t *finished_file_buffer = nullptr;
    int32_t finished_file_buffer_byte_count = 0;

    BLECharacteristic *pFileBlockCharacteristic;
    BLECharacteristic *pFileLengthCharacteristic;
    BLECharacteristic *pFileMaximumLengthCharacteristic;
    BLECharacteristic *pFileTransferTypeCharacteristic;
    BLECharacteristic *pChecksumCharacteristic;
    BLECharacteristic *pCommandCharacteristic;
    BLECharacteristic *pTransferStatusCharacteristic;
    BLECharacteristic *pErrorMessageCharacteristic;

    class FileBlockCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            if (in_progress_file_buffer == nullptr)
            {
                notifyError("File block sent while no valid command is active");
                return;
            }

            uint8_t *data = pCharacteristic->getData();
            size_t size = pCharacteristic->m_value.getLength();

            const int32_t file_block_length = size;
            if (file_block_length > file_block_byte_count)
            {
                notifyError(String("Too many bytes in block: Expected ") + String(file_block_byte_count) +
                            String(" but received ") + String(file_block_length));
                in_progress_file_buffer = nullptr;
                return;
            }

            const int32_t bytes_received_after_block = in_progress_bytes_received + file_block_length;
            if ((bytes_received_after_block > in_progress_bytes_expected) ||
                (bytes_received_after_block > file_maximum_byte_count))
            {
                notifyError(String("Too many bytes: Expected ") + String(in_progress_bytes_expected) +
                            String(" but received ") + String(bytes_received_after_block));
                in_progress_file_buffer = nullptr;
                return;
            }

            uint8_t *file_block_buffer = in_progress_file_buffer + in_progress_bytes_received;
            MEMCPY(file_block_buffer, data, file_block_length);

// Enable this macro to show the data in the serial log.
// #define ENABLE_LOGGING
#ifdef ENABLE_LOGGING
            Serial.print("Data received: length = ");
            Serial.println(file_block_length);

            char string_buffer[file_block_byte_count + 1];
            for (int i = 0; i < file_block_byte_count; ++i)
            {
                unsigned char value = file_block_buffer[i];
                if (i < file_block_length)
                {
                    string_buffer[i] = value;
                }
                else
                {
                    string_buffer[i] = 0;
                }
            }
            string_buffer[file_block_byte_count] = 0;
            Serial.println(String(string_buffer));
#endif // ENABLE_LOGGING

            if (bytes_received_after_block == in_progress_bytes_expected)
            {
                onFileTransferComplete();
            }
            else
            {
                in_progress_bytes_received = bytes_received_after_block;
            }
        }
    };

    class CommandCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t command_value;
            uint8_t *command_value_data = pCharacteristic->getData();
            command_value = command_value_data[0];

            Serial.print("set file transfer command: ");
            Serial.println(command_value);

            switch (command_value)
            {
            case START_FILE_TRANSFER:
                Serial.println("received start_file_transfer command...");
                startFileTransfer();
                break;
            case CANCEL_FILE_TRANSFER:
                Serial.println("received cancel_file_transfer command...");
                cancelFileTransfer();
                break;
            default:
                notifyError(String("Invalid command: ") + String(command_value));
                break;
            }
        }
    };

    class FileTransferTypeCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *data = pCharacteristic->getData();
            unsigned char rawFileTransferType = data[0];
            Serial.print("set file transfer type: ");
            Serial.println(rawFileTransferType);
            fileTransferType = (FileTransferType)rawFileTransferType;
        }
    };

    void setup()
    {
        file_buffers[0] = (uint8_t *)malloc(file_maximum_byte_count * sizeof(uint8_t));
        file_buffers[1] = (uint8_t *)malloc(file_maximum_byte_count * sizeof(uint8_t));

        pFileBlockCharacteristic = ble::createCharacteristic(GENERATE_UUID("3000"), BLECharacteristic::PROPERTY_WRITE, "File Block");
        pFileLengthCharacteristic = ble::createCharacteristic(GENERATE_UUID("3001"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "File Length");
        pFileMaximumLengthCharacteristic = ble::createCharacteristic(GENERATE_UUID("3002"), BLECharacteristic::PROPERTY_READ, "Maximum File Length");
        pFileTransferTypeCharacteristic = ble::createCharacteristic(GENERATE_UUID("3003"), BLECharacteristic::PROPERTY_WRITE, "File Transfer Type");
        pChecksumCharacteristic = ble::createCharacteristic(GENERATE_UUID("3004"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "File Checksum");
        pCommandCharacteristic = ble::createCharacteristic(GENERATE_UUID("3005"), BLECharacteristic::PROPERTY_WRITE, "File Command");
        pTransferStatusCharacteristic = ble::createCharacteristic(GENERATE_UUID("3006"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "File Transfer Status");
        pErrorMessageCharacteristic = ble::createCharacteristic(GENERATE_UUID("3007"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "File Error Message");

        pFileBlockCharacteristic->setCallbacks(new FileBlockCharacteristicCallbacks());
        pFileTransferTypeCharacteristic->setCallbacks(new FileTransferTypeCharacteristicCallbacks());
        pCommandCharacteristic->setCallbacks(new CommandCharacteristicCallbacks());

        uint16_t fileMaximumLengthCharacteristicData[1] = {file_maximum_byte_count};
        pFileMaximumLengthCharacteristic->setValue((uint8_t *)fileMaximumLengthCharacteristicData, sizeof(uint16_t));

        uint8_t pTransferStatusCharacteristicData[1] = {SUCCESS_STATUS};
        pTransferStatusCharacteristic->setValue((uint8_t *)pTransferStatusCharacteristicData, sizeof(uint8_t));
    }

    bool isTransfering()
    {
        return in_progress_file_buffer != nullptr;
    }
    void startFileTransfer()
    {
        Serial.println("start file transfer");

        if (in_progress_file_buffer != nullptr)
        {
            notifyError("File transfer command received while previous transfer is still in progress");
            return;
        }

        uint8_t *file_length_data = pFileLengthCharacteristic->getData();
        int32_t file_length_value = ((int32_t)file_length_data[0]) | (((int32_t)file_length_data[1])) << 8 | ((int32_t)file_length_data[2]) << 16 | ((int32_t)file_length_data[3]) << 24;
        if (file_length_value > file_maximum_byte_count)
        {
            notifyError(
                String("File too large: Maximum is ") + String(file_maximum_byte_count) +
                String(" bytes but request is ") + String(file_length_value) + String(" bytes"));
            return;
        }

        uint8_t *in_progress_checksum_data = pChecksumCharacteristic->getData();
        in_progress_checksum = ((int32_t)in_progress_checksum_data[0]) | (((int32_t)in_progress_checksum_data[1])) << 8 | ((int32_t)in_progress_checksum_data[2]) << 16 | ((int32_t)in_progress_checksum_data[3]) << 24;

        int in_progress_file_buffer_index;
        if (finished_file_buffer_index == 0)
        {
            in_progress_file_buffer_index = 1;
        }
        else
        {
            in_progress_file_buffer_index = 0;
        }

        in_progress_file_buffer = &file_buffers[in_progress_file_buffer_index][0];
        in_progress_bytes_received = 0;
        in_progress_bytes_expected = file_length_value;

        notifyInProgress();
    }
    void cancelFileTransfer()
    {
        if (in_progress_file_buffer != nullptr)
        {
            notifyError("File transfer cancelled");
            in_progress_file_buffer = nullptr;
        }
    }
    void onFileTransferComplete()
    {
        uint32_t computed_checksum = crc32(in_progress_file_buffer, in_progress_bytes_expected);
        ;
        if (in_progress_checksum != computed_checksum)
        {
            notifyError(String("File transfer failed: Expected checksum 0x") + String(in_progress_checksum, 16) +
                        String(" but received 0x") + String(computed_checksum, 16));
            in_progress_file_buffer = nullptr;
            return;
        }

        if (finished_file_buffer_index == 0)
        {
            finished_file_buffer_index = 1;
        }
        else
        {
            finished_file_buffer_index = 0;
        }

        finished_file_buffer = &file_buffers[finished_file_buffer_index][0];
        finished_file_buffer_byte_count = in_progress_bytes_expected;

        in_progress_file_buffer = nullptr;
        in_progress_bytes_received = 0;
        in_progress_bytes_expected = 0;

        notifySuccess();

        onFileReceived(finished_file_buffer, finished_file_buffer_byte_count);
    }
    void onFileReceived(uint8_t *file_data, int file_length)
    {
        Serial.print("file received with type: ");
        Serial.println(fileTransferType);
        
        switch (fileTransferType)
        {
        case TF_LITE_MODEL_FILE:
            tfLite::loadModel(file_data);
            break;
        }
    }

    void notifySuccess()
    {
        uint8_t success_status_code_buffer[1];
        success_status_code_buffer[0] = SUCCESS_STATUS;

        pTransferStatusCharacteristic->setValue((uint8_t *)success_status_code_buffer, sizeof(uint8_t));

        if (ble::isServerConnected) {
            pTransferStatusCharacteristic->notify();
        }
    }
    void notifyInProgress()
    {
        uint8_t in_progress_status_code_buffer[1];
        in_progress_status_code_buffer[0] = IN_PROGRESS_STATUS;

        pTransferStatusCharacteristic->setValue((uint8_t *)in_progress_status_code_buffer, sizeof(uint8_t));
        if (ble::isServerConnected) {
            pTransferStatusCharacteristic->notify();
        }
    }
    void notifyError(const String &error_message)
    {
        uint8_t error_status_code_buffer[1];
        error_status_code_buffer[0] = ERROR_STATUS;

        Serial.print("ERROR: ");
        Serial.println(error_message.c_str());

        pTransferStatusCharacteristic->setValue((uint8_t *)error_status_code_buffer, sizeof(uint8_t));
        if (ble::isServerConnected) {
            pTransferStatusCharacteristic->notify();
        }

        pErrorMessageCharacteristic->setValue(error_message.c_str());
        if (ble::isServerConnected) {
            pErrorMessageCharacteristic->notify();
        }
    }

    uint32_t crc32_for_byte(uint32_t r)
    {
        for (int j = 0; j < 8; ++j)
        {
            r = (r & 1 ? 0 : (uint32_t)0xedb88320L) ^ r >> 1;
        }
        return r ^ (uint32_t)0xff000000L;
    }

    uint32_t crc32(const uint8_t *data, size_t data_length)
    {
        constexpr int table_size = 256;
        static uint32_t table[table_size];
        static bool is_table_initialized = false;
        if (!is_table_initialized)
        {
            for (size_t i = 0; i < table_size; ++i)
            {
                table[i] = crc32_for_byte(i);
            }
            is_table_initialized = true;
        }
        uint32_t crc = 0;
        for (size_t i = 0; i < data_length; ++i)
        {
            const uint8_t crc_low_byte = static_cast<uint8_t>(crc);
            const uint8_t data_byte = data[i];
            const uint8_t table_index = crc_low_byte ^ data_byte;
            crc = table[table_index] ^ (crc >> 8);
        }
        return crc;
    }
} // namespace fileTransfer