#pragma once
#ifndef _HAPTICS_
#define _HAPTICS_

#include "Adafruit_DRV2605.h"

namespace haptics
{    
    void vibrate(uint8_t *data, size_t length);

    void setup();
    void loop();
} // namespace haptics

#endif // _HAPTICS_