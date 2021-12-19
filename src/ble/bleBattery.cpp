#include "bleBattery.h"
#include "battery.h"

namespace bleBattery
{
    BLEService *pService;
    BLECharacteristic *pLevelCharacteristic;

    void updateLevelCharacteristic(bool notify = false)
    {
        pLevelCharacteristic->setValue(100); // FIX LATER
        if (notify)
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
        
    }
} // namespace bleBattery