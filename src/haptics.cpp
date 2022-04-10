#include "haptics.h"
#include "information/type.h"

namespace haptics
{
    enum class VibrationType : uint8_t
    {
        WAVEFORM = 0,
        SEQUENCE,
        COUNT
    };

    constexpr uint8_t max_waveform_length = 8;
    constexpr uint8_t max_sequence_length = 20;

    Adafruit_DRV2605 drv;
    bool didSetup = false;
    bool canBegin() {
        return !didSetup && type::isInsole();
    }
    void _begin() {
        if (canBegin())
        {
            if (drv.begin()) {
                drv.begin();
                drv.setMode(DRV2605_MODE_INTTRIG);
                drv.selectLibrary(1);

                Serial.println("started haptics");
                didSetup = true;
            }
            else {
                Serial.println("unable to start haptics");
            }
        }
    }

    bool shouldBegin = false;
    void begin() {
        if (canBegin()) {
            shouldBegin = true;
        }
    }

    void setup()
    {
        _begin();
    }

    unsigned long currentTime;

    uint8_t waveform[max_waveform_length]{0};
    bool shouldTriggerWaveform = false;
    void triggerWaveform()
    {
        drv.stop();

        drv.setMode(DRV2605_MODE_INTTRIG);
        drv.selectLibrary(1);

        for (uint8_t waveformSlotIndex = 0; waveformSlotIndex < max_waveform_length; waveformSlotIndex++)
        {
            uint8_t waveformValue = waveform[waveformSlotIndex];
            drv.setWaveform(waveformSlotIndex, waveformValue);
            // Serial.printf("%u: %u\n", waveformSlotIndex, waveformValue);
        }
        drv.go();
    }

    bool shouldTriggerSequence = false;
    bool isTriggeringSequence = false;
    uint8_t sequence[max_sequence_length]{0}; // [amplitude, delay, amplitude, delay...]
    int sequenceIndex = -1;
    uint8_t sequenceLength = 0;
    void triggerSequence()
    {   
        drv.stop();
        isTriggeringSequence = true;
        sequenceIndex = -1;

        drv.setMode(DRV2605_MODE_REALTIME);
    }
    unsigned long nextSequenceUpdate = 0;
    void continueSequence()
    {
        bool isFinished = false;

        bool shouldUpdateValue = false;
        int8_t newSequenceIndex;

        if (sequenceIndex == -1)
        {
            newSequenceIndex = 0;
            shouldUpdateValue = true;
        }
        else
        {
            if (currentTime >= nextSequenceUpdate)
            {
                newSequenceIndex = sequenceIndex + 2;
                shouldUpdateValue = true;
            }
        }

        // check if we've reached the end or we have a zero delay (invalid)
        if (shouldUpdateValue && (newSequenceIndex >= sequenceLength || sequence[newSequenceIndex + 1] == 0))
        {
            shouldUpdateValue = false;
            isFinished = true;
        }

        if (shouldUpdateValue)
        {
            // Serial.printf("new sequence index: %d\n", newSequenceIndex);
            sequenceIndex = newSequenceIndex;
            drv.setRealtimeValue(sequence[sequenceIndex]);
            nextSequenceUpdate = currentTime + (sequence[sequenceIndex + 1] * 10);
        }

        if (isFinished)
        {
            // Serial.println("finished sequence");
            isTriggeringSequence = false;
            drv.stop();
            drv.setMode(DRV2605_MODE_INTTRIG);
            drv.selectLibrary(1);
        }
    }

    void vibrate(uint8_t *data, size_t length)
    {
        if (!didSetup) {
            return;
        }

        auto vibrationType = (VibrationType)data[0];
        switch (vibrationType)
        {
        case VibrationType::WAVEFORM:
            for (uint8_t waveformSlotIndex = 0; waveformSlotIndex < max_waveform_length; waveformSlotIndex++)
            {
                uint8_t waveformValue = (waveformSlotIndex < (length - 1)) ? data[waveformSlotIndex + 1] : 0;
                waveform[waveformSlotIndex] = waveformValue;
                // Serial.printf("%u: %u\n", waveformSlotIndex, waveformValue);
            }
            shouldTriggerWaveform = true;
            break;
        case VibrationType::SEQUENCE:
            sequenceLength = (length - 1 < max_sequence_length) ? (length - 1) : max_sequence_length;
            for (uint8_t sequenceIndex = 0; sequenceIndex < max_sequence_length; sequenceIndex++)
            {
                uint8_t sequenceValue = (sequenceIndex < (length - 1)) ? data[sequenceIndex + 1] : 0;
                sequence[sequenceIndex] = sequenceValue;
                //Serial.printf("%u: %u\n", sequenceIndex, sequenceValue);
            }
            if (sequenceLength > 0)
            {
                shouldTriggerSequence = true;
            }
            break;
        default:
            Serial.printf("uncaught vibration type %u\n", (uint8_t)vibrationType);
            break;
        }
    }

    void loop()
    {
        currentTime = millis();

        if (shouldBegin) {
            _begin();
            shouldBegin = false;
        }

        if (shouldTriggerWaveform)
        {
            triggerWaveform();
            shouldTriggerWaveform = false;
        }

        if (shouldTriggerSequence)
        {
            triggerSequence();
            shouldTriggerSequence = false;
        }
        if (isTriggeringSequence)
        {
            continueSequence();
        }
    }
} // namespace haptics