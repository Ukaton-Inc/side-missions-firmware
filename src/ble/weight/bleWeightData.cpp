#include "bleWeightData.h"
#include "weight/weightData.h"
#include "wifi/webSocket.h"

namespace bleWeightData
{
    BLECharacteristic *pDelayCharacteristic;
    void updateDelayCharacteristic()
    {
        auto delay = weightData::getDelay();
        pDelayCharacteristic->setValue(delay);
    }
    class DelayCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue();
            auto rawData = (uint16_t *) data.c_str();
            auto newDelay = rawData[0];
            weightData::setDelay(newDelay);
            updateDelayCharacteristic();
        }
    };

    void clearDelay()
    {
        weightData::setDelay(0);
        updateDelayCharacteristic();
    }

    BLECharacteristic *pDataCharacteristic;
    unsigned long lastDataUpdateTime;
    void updateDataCharacteristic()
    {
        auto weight = weightData::getWeight();
        pDataCharacteristic->setValue(weight);
        pDataCharacteristic->notify();
    }

    void setup()
    {
        pDelayCharacteristic = ble::createCharacteristic(GENERATE_UUID("8001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "weight data configuration");
        pDelayCharacteristic->setCallbacks(new DelayCharacteristicCallbacks());
        updateDelayCharacteristic();
        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("8002"), NIMBLE_PROPERTY::NOTIFY, "weight data");
    }

    void loop()
    {
        if (lastDataUpdateTime != weightData::lastDataUpdateTime && !webSocket::isConnectedToClient())
        {
            lastDataUpdateTime = weightData::lastDataUpdateTime;
            updateDataCharacteristic();
        }
    }
} // namespace bleWeightData