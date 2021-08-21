#include "wearableDevice.h"

namespace wearableDevice
{
    uint8_t information[19] = {4, 0, 64, 44, 1, 0, 0, 0, 15, 0, 7, 0, 14, 8, 0, 47, 4, 0, 0};
    BLECharacteristic *pInformationCharacteristic;
    void setup()
    {
        pInformationCharacteristic = ble::createCharacteristic("7B61AD83-041C-4333-A0AB-EFB2AB7BDD43", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "Device information");
        pInformationCharacteristic->setValue((uint8_t *) information, sizeof(information));
    }
} // namespace wearableDevice