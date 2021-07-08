#pragma once
#ifndef _EEPROM_UTILS_
#define _EEPROM_UTILS_

#include <EEPROM.h>

#define EEPROM_SIZE 512
#define EEPROM_SCHEMA 1

namespace eepromUtils
{
    extern unsigned char schema;
    extern bool firstInitialized;
    extern uint16_t freeAddress;
    void setup();
    uint16_t getAvailableSpace();
    uint16_t reserveSpace(uint16_t size);
}

#endif // _EEPROM_UTILS_