#include "haptics.h"
#include "Adafruit_DRV2605.h"

namespace haptics
{
    Adafruit_DRV2605 drv;
    void setup() {
        drv.begin();
        drv.selectLibrary(1);
        drv.setMode(DRV2605_MODE_INTTRIG); 
    }

    uint8_t effect = 0;
    unsigned long lastWaveformPlayTime = 0;
    constexpr uint16_t play_waveform_delay_ms = 500;
    void playWaveform() {
        drv.setWaveform(0, effect);  // play effect 
        drv.setWaveform(1, 0);
        effect = effect+1 % 124;
    }

    unsigned long currentTime;
    void loop() {
        currentTime = millis();
        if (currentTime >= lastWaveformPlayTime + play_waveform_delay_ms)
        {
            lastWaveformPlayTime = currentTime - (currentTime % play_waveform_delay_ms);
            playWaveform();
        }
    }
} // namespace haptics
