#pragma once
#ifndef _FOCUS_IDENTIFIER_
#define _FOCUS_IDENTIFIER_

#include "ble.h"

namespace focusIdentifier
{
    extern uint8_t focusIdentifier[4];
    extern BLECharacteristic *pFocusIdentifierCharacteristic;
    void setup();
}

#endif // _FOCUS_IDENTIFIER_