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

            didSetup = true;
        }
    }
} // namespace haptics