#pragma once
#ifndef _NAME_
#define _NAME_

#include <Arduino.h>
#include "ble.h"

namespace name
{
    extern const uint8_t MAX__NAME_LENGTH;

    extern std::string name;
    extern uint16_t eepromAddress;

    extern BLECharacteristic *pCharacteristic;
    class CharacteristicCallbacks;

    void setup();
    void loadFromEEPROM();
    void saveToEEPROM();
    void setName(char* newName);
} // namespace name

#endif // _NAME_