#include "haptics.h"
#include "information/type.h"

namespace haptics
{
    Adafruit_DRV2605 drv;
    bool didSetup = false;
    void setup()
    {
        if (!didSetup && type::isInsole())
        {
            drv.begin();
            drv.setMode(DRV2605_MODE_INTTRIG);
            drv.selectLibrary(1);
            
            drv.setWaveform(0, 84);  // ramp up medium 1, see datasheet part 11.2
            drv.setWaveform(1, 0);  // end of waveforms

            didSetup = true;
        }
    }
} // namespace haptics