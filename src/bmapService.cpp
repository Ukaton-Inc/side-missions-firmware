#include "bmapService.h"

namespace bmapService
{
    BLEService *pService;

    BLECharacteristic *pCharacteristic1;
    BLECharacteristic *pCharacteristic2;
    BLECharacteristic *pCharacteristic3;
    BLECharacteristic *pCharacteristic4;

    void setup()
    {        
        pService = ble::pServer->createService(BLEUUID("0000febe-0000-1000-8000-00805f9b34fb"), 256);
        
        pCharacteristic1 = ble::createCharacteristic("234bfbd5-e3b3-4536-a3fe-723620d4b78d", NIMBLE_PROPERTY::WRITE, "bmap characteristic 1", pService);
        pCharacteristic2 = ble::createCharacteristic("9ec813b4-256b-4090-93a8-a4f0e9107733", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "bmap characteristic 2", pService);
        uint8_t data2[6] = {0};
        pCharacteristic2->setValue(data2, sizeof(data2));
        pCharacteristic3 = ble::createCharacteristic("d417c028-9818-4354-99d1-2ac09d074591", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::WRITE_NR | NIMBLE_PROPERTY::NOTIFY, "bmap characteristic 3", pService);
        pCharacteristic4 = ble::createCharacteristic("eecb95dd-befe-40d6-98c7-0651d2d09ba9", NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY, "bmap characteristic 4", pService);
    }

    void start()
    {
        pService->start();
    }
} // namespace bmapService