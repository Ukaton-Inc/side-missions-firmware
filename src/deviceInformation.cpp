#include "deviceInformation.h"

namespace deviceInformation
{
    BLEService *pService;

    BLECharacteristic *pSystemIDCharacteristic;
    BLECharacteristic *pModelNumberStringCharacteristic;
    BLECharacteristic *pSerialNumberStringCharacteristic;
    BLECharacteristic *pFirmwareRevisionStringCharacteristic;
    BLECharacteristic *pHardwareRevisionStringCharacteristic;
    BLECharacteristic *pSoftwareRevisionStringCharacteristic;
    BLECharacteristic *pManufacturerNameStringCharacteristic;
    BLECharacteristic *pPnpIDCharacteristic;

    void setup()
    {        
        pService = ble::pServer->createService(BLEUUID("0000180a-0000-1000-8000-00805f9b34fb"), 256);
        
        pSystemIDCharacteristic = ble::createCharacteristic("00002a23-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::READ, "System ID", pService);
        uint8_t systemIdData[8] = {0, 0, 0, 0, 0, 31, 223, 8};
        pSystemIDCharacteristic->setValue(systemIdData, sizeof(systemIdData));
        pModelNumberStringCharacteristic = ble::createCharacteristic("00002a24-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::READ, "Model number", pService);
        pModelNumberStringCharacteristic->setValue("Motion Module");
        pSerialNumberStringCharacteristic = ble::createCharacteristic("00002a25-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::READ, "Serial number", pService);
        pSerialNumberStringCharacteristic->setValue("Concrete Science fiction");
        pFirmwareRevisionStringCharacteristic = ble::createCharacteristic("00002a26-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::READ, "Firmware revision", pService);
        pFirmwareRevisionStringCharacteristic->setValue("4.1.6");
        pHardwareRevisionStringCharacteristic = ble::createCharacteristic("00002a27-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::READ, "Hardware revision", pService);
        pHardwareRevisionStringCharacteristic->setValue("0");
        pSoftwareRevisionStringCharacteristic = ble::createCharacteristic("00002a28-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::READ, "Software revision", pService);
        pSoftwareRevisionStringCharacteristic->setValue("4.1.6");
        pManufacturerNameStringCharacteristic = ble::createCharacteristic("00002a29-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::READ, "Manufactuerer name", pService);
        pManufacturerNameStringCharacteristic->setValue("Ukaton");
        pPnpIDCharacteristic = ble::createCharacteristic("00002a50-0000-1000-8000-00805f9b34fb", NIMBLE_PROPERTY::READ, "Pnp id", pService);
        uint8_t pnpIdData[7] = {1, 158, 0, 44, 64, 22, 4};
        pPnpIDCharacteristic->setValue(pnpIdData, sizeof(pnpIdData));
    }

    void start()
    {
        pService->start();
    }
} // namespace ble