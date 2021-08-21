#pragma once
#ifndef _BMAP_SERVICE_
#define _BMAP_SERVICE_

#include "ble.h"

namespace bmapService
{
    extern BLEService *pService;

    extern BLECharacteristic *pCharacteristic1;
    extern BLECharacteristic *pCharacteristic2;
    extern BLECharacteristic *pCharacteristic3;
    extern BLECharacteristic *pCharacteristic4;

    void setup();
    void start();
} // namespace bmapService

#endif // _BMAP_SERVICE_