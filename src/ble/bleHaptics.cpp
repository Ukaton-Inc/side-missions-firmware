#include "bleHaptics.h"
#include "haptics.h"

namespace bleHaptics
{
    uint8_t waveform[haptics::max_number_of_waveforms]{0};
    bool shouldTriggerWaveform = false;
    void triggerWaveform()
    {
        for (uint8_t waveformSlotIndex = 0; waveformSlotIndex < haptics::max_number_of_waveforms; waveformSlotIndex++)
        {
            uint8_t waveformValue = waveform[waveformSlotIndex];
            haptics::drv.setWaveform(waveformSlotIndex, waveformValue);
            Serial.printf("%u: %u\n", waveformSlotIndex, waveformValue);
        }
        Serial.println("triggering");
        haptics::drv.go();
    }

    unsigned long lastTimeWaveformWasTriggered = 0;

    BLECharacteristic *pWaveformCharacteristic;
    class WaveformCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            unsigned long currentTime = millis();

            if (currentTime - lastTimeWaveformWasTriggered > haptics::waveform_trigger_delay_ms)
            {
                auto value = pCharacteristic->getValue();

                auto length = value.length();
                auto data = value.data();

                for (uint8_t waveformSlotIndex = 0; waveformSlotIndex < haptics::max_number_of_waveforms; waveformSlotIndex++)
                {
                    uint8_t waveformValue = (waveformSlotIndex < length) ? data[waveformSlotIndex] : 0;
                    waveform[waveformSlotIndex] = waveformValue;
                    Serial.printf("%u: %u\n", waveformSlotIndex, waveformValue);
                }

                shouldTriggerWaveform = true;
                lastTimeWaveformWasTriggered = currentTime;
            }
        }
    };

    void setup()
    {
        pWaveformCharacteristic = ble::createCharacteristic(GENERATE_UUID("d000"), NIMBLE_PROPERTY::WRITE, "haptics waveform");
        pWaveformCharacteristic->setCallbacks(new WaveformCharacteristicCallbacks());
    }

    void loop()
    {
        if (shouldTriggerWaveform)
        {
            triggerWaveform();
            shouldTriggerWaveform = false;
        }
    }
} // namespace bleHaptics
