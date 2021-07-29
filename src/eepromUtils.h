#pragma once
#ifndef _EEPROM_UTILS_
#define _EEPROM_UTILS_

#include <EEPROM.h>

namespace eepromUtils
{
    extern const uint16_t EEPROM_SIZE;
    extern const uint8_t EEPROM_SCHEMA;
    extern unsigned char schema;
    extern bool firstInitialized;
    extern uint16_t freeAddress;
    void setup();
    uint16_t getAvailableSpace();
    uint16_t reserveSpace(uint16_t size);
}

#endif // _EEPROM_UTILS_