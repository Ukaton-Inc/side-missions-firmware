#include "definitions.h"
#include "bleFirmwareUpdate.h"

#include <Update.h>

namespace bleFirmwareUpdate
{
    BLECharacteristic *pVersionCharacteristic;
    BLECharacteristic *pMaxSizeCharacteristic;
    BLECharacteristic *pDataCharacteristic;

    void onProgress(size_t progress, size_t size) {
        //Serial.printf("progress: %d, size: %d\n", progress, size);
    }

    class DataCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto value = pCharacteristic->getValue();

            if (!Update.isRunning())
            {
                if (!Update.begin(UPDATE_SIZE_UNKNOWN))
                {
                    Update.printError(Serial);
                }
            }

            Update.write((uint8_t *) value.data(), value.length());

            if (value.length() != block_byte_count)
            {
                if (Update.isRunning())
                {
                    Serial.println("OTA done!");
                    if (Update.end(true))
                    {
                        if (Update.isFinished())
                        {
                            Serial.println("Update successfully completed. Rebooting.");
                            ESP.restart();
                        }
                        else
                        {
                            Serial.println("Update not finished? Something went wrong!");
                        }
                    }
                    else
                    {
                        Update.printError(Serial);
                    }
                }
            }
        }
    };

    std::string version = "0.0";

    void setup()
    {
        pVersionCharacteristic = ble::createCharacteristic(GENERATE_UUID("b000"), NIMBLE_PROPERTY::READ, "firmware version");
        pVersionCharacteristic->setValue(version);

        pMaxSizeCharacteristic = ble::createCharacteristic(GENERATE_UUID("b001"), NIMBLE_PROPERTY::READ, "max firmware size");
        pMaxSizeCharacteristic->setValue(max_size);

        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("b002"), NIMBLE_PROPERTY::WRITE, "firmware data");
        pDataCharacteristic->setCallbacks(new DataCharacteristicCallbacks());

        Update.onProgress(onProgress);
    }
} // namespace bleFirmwareUpdate