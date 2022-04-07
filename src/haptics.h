#pragma once
#ifndef _HAPTICS_
#define _HAPTICS_

#include "Adafruit_DRV2605.h"

namespace haptics
{
    constexpr uint16_t waveform_trigger_delay_ms = 500;
    constexpr uint8_t max_number_of_waveforms = 8;
    extern Adafruit_DRV2605 drv;
    void setup();
} // namespace haptics

#endif // _HAPTICS_