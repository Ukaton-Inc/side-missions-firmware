#include "bleSensorData.h"
#include "sensor/sensorData.h"
#include "wifi/webSocket.h"
#include "definitions.h"

namespace bleSensorData
{
    BLECharacteristic *pConfigurationCharacteristic;
    uint8_t configuration[sizeof(sensorData::motionConfiguration) + sizeof(sensorData::pressureConfiguration)];
    void updateConfigurationCharacteristic()
    {
        uint8_t offset = 0;

        memcpy(&configuration[offset], sensorData::motionConfiguration, sizeof(sensorData::motionConfiguration));
        offset += sizeof(sensorData::motionConfiguration);
        memcpy(&configuration[offset], sensorData::pressureConfiguration, sizeof(sensorData::pressureConfiguration));
        offset += sizeof(sensorData::pressureConfiguration);

        pConfigurationCharacteristic->setValue((uint8_t *)configuration, sizeof(configuration));
    }
    class ConfigurationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue();
#if DEBUG
            Serial.println("received sensor data config ble message:");
            auto rawData = data.data();
            auto length = data.length();
            for (uint8_t index = 0; index < length; index++)
            {
                Serial.printf("%u: %u\n", index, rawData[index]);
            }
#endif
            sensorData::setConfigurations((const uint8_t *)data.data(), (uint8_t)data.length());
            updateConfigurationCharacteristic();
        }
    };

    void clearConfigurations()
    {
        sensorData::clearConfigurations();
        updateConfigurationCharacteristic();
    }

    BLECharacteristic *pDataCharacteristic;
    unsigned long lastDataUpdateTime;
    uint8_t data[2 + sizeof(sensorData::motionData) + 2 + sizeof(sensorData::pressureData)]; // [sensorType, dataSize, payload]x2
    uint8_t dataSize = 0;
    void updateDataCharacteristic()
    {
        dataSize = 0;

        // not including timestamp
        /*
        uint16_t timestamp = (uint16_t) lastDataUpdateTime;
        MEMCPY(&data[dataSize], &timestamp, sizeof(timestamp));
        dataSize += sizeof(timestamp);
        */

        data[dataSize++] = sensorData::motionDataBitmask;
        // data[dataSize++] = (uint8_t) sensorData::SensorType::MOTION;
        // data[dataSize++] = sensorData::motionDataSize;
        memcpy(&data[dataSize], sensorData::motionData, sensorData::motionDataSize);
        dataSize += sensorData::motionDataSize;

        data[dataSize++] = sensorData::pressureDataBitmask;
        // data[dataSize++] = (uint8_t) sensorData::SensorType::PRESSURE;
        // data[dataSize++] = sensorData::pressureDataSize;
        memcpy(&data[dataSize], sensorData::pressureData, sensorData::pressureDataSize);
        dataSize += sensorData::pressureDataSize;

        pDataCharacteristic->setValue(data, dataSize);
        pDataCharacteristic->notify();
    }

    void setup()
    {
        pConfigurationCharacteristic = ble::createCharacteristic(GENERATE_UUID("6001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "sensor data configuration");
        pConfigurationCharacteristic->setCallbacks(new ConfigurationCharacteristicCallbacks());
        updateConfigurationCharacteristic();
        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("6002"), NIMBLE_PROPERTY::NOTIFY, "sensor data");
    }

    void loop()
    {
        if (lastDataUpdateTime != sensorData::lastDataUpdateTime && pDataCharacteristic->getSubscribedCount() > 0 && (sensorData::motionDataSize + sensorData::pressureDataSize > 0) && !webSocket::isConnectedToClient())
        {
            lastDataUpdateTime = sensorData::lastDataUpdateTime;
            updateDataCharacteristic();
        }
    }
} // namespace bleSensorData