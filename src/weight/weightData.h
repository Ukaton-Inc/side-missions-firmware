#pragma once
#ifndef _WEIGHT_DATA_
#define _WEIGHT_DATA_

#include <inttypes.h>

namespace weightData
{
    void setDelay(uint16_t delay);
    uint16_t getDelay();
    float getWeight();
    extern unsigned long lastDataUpdateTime;
    void loop();
} // namespace weightData

#endif // _WEIGHT_DATA_