#include "bleHaptics.h"
#include "haptics.h"

namespace bleHaptics
{
    unsigned long currentTime;

    uint8_t waveform[haptics::max_number_of_waveforms]{0};
    bool shouldTriggerWaveform = false;
    void triggerWaveform()
    {
        haptics::drv.stop();

        haptics::drv.setMode(DRV2605_MODE_INTTRIG);
        haptics::drv.selectLibrary(1);

        for (uint8_t waveformSlotIndex = 0; waveformSlotIndex < haptics::max_number_of_waveforms; waveformSlotIndex++)
        {
            uint8_t waveformValue = waveform[waveformSlotIndex];
            haptics::drv.setWaveform(waveformSlotIndex, waveformValue);
            //Serial.printf("%u: %u\n", waveformSlotIndex, waveformValue);
        }
        haptics::drv.go();
    }

    bool shouldTriggerSequence = false;
    bool isTriggeringSequence = false;
    uint8_t sequence[haptics::max_sequence_length]{0}; // [amplitude, delay, amplitude...]
    int sequenceIndex = -1;
    uint8_t sequenceLength = 0;
    void triggerSequence() {
        haptics::drv.stop();
        isTriggeringSequence = true;
        sequenceIndex = -1;
        
        haptics::drv.setMode(DRV2605_MODE_REALTIME);
    }
    unsigned long nextSequenceUpdate = 0;
    void continueSequence() {
        bool isFinished = false;

        bool shouldUpdateValue = false;
        int8_t newSequenceIndex;

        if (sequenceIndex == -1) {
            newSequenceIndex = 0;
            shouldUpdateValue = true;
        }
        else {
            if (currentTime >= nextSequenceUpdate) {
                newSequenceIndex = sequenceIndex+2;
                shouldUpdateValue = true;
            }
        }

        // check if we've reached the end or we have a zero delay (invalid)
        if (shouldUpdateValue && (newSequenceIndex >= sequenceLength || sequence[newSequenceIndex+1] == 0)) {
            shouldUpdateValue = false;
            isFinished = true;
        }

        if (shouldUpdateValue) {
            //Serial.printf("new sequence index: %d\n", newSequenceIndex);
            sequenceIndex = newSequenceIndex;
            haptics::drv.setRealtimeValue(sequence[sequenceIndex]);
            nextSequenceUpdate = currentTime + (sequence[sequenceIndex+1]*10);
        }

        if (isFinished) {
            //Serial.println("finished sequence");
            isTriggeringSequence = false;
            haptics::drv.stop();
            haptics::drv.setMode(DRV2605_MODE_INTTRIG);
            haptics::drv.selectLibrary(1);
        }
    }

    BLECharacteristic *pVibrationCharacteristic;
    class VibrationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto value = pCharacteristic->getValue();

            auto length = value.length();
            auto data = value.data();

            auto vibrationType = (VibrationType) data[0];
            switch (vibrationType) {
                case VibrationType::WAVEFORM:
                for (uint8_t waveformSlotIndex = 0; waveformSlotIndex < haptics::max_number_of_waveforms; waveformSlotIndex++)
                {
                    uint8_t waveformValue = (waveformSlotIndex < (length-1)) ? data[waveformSlotIndex+1] : 0;
                    waveform[waveformSlotIndex] = waveformValue;
                    //Serial.printf("%u: %u\n", waveformSlotIndex, waveformValue);
                }
                shouldTriggerWaveform = true;
                break;
                case VibrationType::SEQUENCE:
                sequenceLength = (length-1 < haptics::max_sequence_length)? (length-1) : haptics::max_sequence_length;
                for (uint8_t sequenceIndex = 0; sequenceIndex < haptics::max_sequence_length; sequenceIndex++)
                {
                    uint8_t sequenceValue = (sequenceIndex < (length-1)) ? data[sequenceIndex+1] : 0;
                    sequence[sequenceIndex] = sequenceValue;
                    //Serial.printf("%u: %u\n", sequenceIndex, sequenceValue);
                }
                if (sequenceLength > 0) {
                    shouldTriggerSequence = true;
                }
                break;
                default:
                Serial.printf("uncaught vibration type %u\n", (uint8_t) vibrationType);
                break;
            }

        }
    };

    void setup()
    {
        pVibrationCharacteristic = ble::createCharacteristic(GENERATE_UUID("d000"), NIMBLE_PROPERTY::WRITE, "haptics vibration");
        pVibrationCharacteristic->setCallbacks(new VibrationCharacteristicCallbacks());
    }

    void loop()
    {
        currentTime = millis();

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
        if (isTriggeringSequence) {
            continueSequence();
        }
    }
} // namespace bleHaptics
