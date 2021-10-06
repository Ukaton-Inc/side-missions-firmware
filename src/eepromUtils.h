#pragma once
#ifndef _EEPROM_UTILS_
#define _EEPROM_UTILS_

#include <EEPROM.h>

namespace eepromUtils
{
    extern bool firstInitialized;
    void setup();
    uint16_t getAvailableSpace();
    uint16_t reserveSpace(uint16_t size);
}

#endif // _EEPROM_UTILS_