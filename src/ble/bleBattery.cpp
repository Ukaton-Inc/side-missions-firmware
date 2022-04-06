#include "bleBattery.h"
#include "battery.h"

namespace bleBattery
{
    BLEService *pService;
    BLECharacteristic *pLevelCharacteristic;

    unsigned long lastUpdateBatteryLevelTime = 0;
    void updateLevelCharacteristic(bool notify = false)
    {
        return;
        uint8_t batteryLevel = battery::soc;
        pLevelCharacteristic->setValue(batteryLevel);
        if (notify && pLevelCharacteristic->getSubscribedCount() > 0)
        {
            pLevelCharacteristic->notify();
        }
    }

    void setup()
    {
        pService = ble::pServer->createService(BLEUUID((uint16_t)0x180F));
        pLevelCharacteristic = ble::createCharacteristic(BLEUUID((uint16_t)0x2A19), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "Battery Level", pService);
        updateLevelCharacteristic();
        pService->start();
    }

    void loop() {
        if (lastUpdateBatteryLevelTime != battery::lastUpdateBatteryLevelTime) {
            updateLevelCharacteristic(true);
            lastUpdateBatteryLevelTime = battery::lastUpdateBatteryLevelTime;
        }
    }
} // namespace bleBattery