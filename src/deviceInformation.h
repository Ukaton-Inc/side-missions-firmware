#pragma once
#ifndef _DEVICE_INFORMATION_
#define _DEVICE_INFORMATION_

#include "ble.h"

namespace deviceInformation
{
    extern BLEService *pService;

    extern BLECharacteristic *pSystemIDCharacteristic;
    extern BLECharacteristic *pModelNumberStringCharacteristic;
    extern BLECharacteristic *pSerialNumberStringCharacteristic;
    extern BLECharacteristic *pFirmwareRevisionStringCharacteristic;
    extern BLECharacteristic *pHardwareRevisionStringCharacteristic;
    extern BLECharacteristic *pSoftwareRevisionStringCharacteristic;
    extern BLECharacteristic *pManufacturerNameStringCharacteristic;
    extern BLECharacteristic *pPnpIDCharacteristic;

    void setup();
    void start();
} // namespace deviceInformation

#endif // _DEVICE_INFORMATION_