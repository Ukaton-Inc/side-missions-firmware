#include "bleBattery.h"
#include "battery.h"

namespace bleBattery
{
    BLEService *pService;
    BLECharacteristic *pLevelCharacteristic;

    bool isSubscribed = false;
    class LevelCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue)
        {
            if (subValue == 0) {
                isSubscribed = false;
            }
            else if (subValue == 1) {
                isSubscribed = true;
            }
        }
    };

    void updateLevelCharacteristic(bool notify = false)
    {
        pLevelCharacteristic->setValue(100); // FIX LATER
        if (isSubscribed && notify)
        {
            pLevelCharacteristic->notify();
        }
    }

    void setup()
    {
        pService = ble::pServer->createService(BLEUUID((uint16_t)0x180F));
        pLevelCharacteristic = ble::createCharacteristic(BLEUUID((uint16_t)0x2A19), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "Battery Level", pService);
        pLevelCharacteristic->setCallbacks(new LevelCharacteristicCallbacks());
        updateLevelCharacteristic();
        pService->start();
    }

    void loop() {
        // FIX LATER
    }
} // namespace bleBattery