#include "bleHaptics.h"
#include "haptics.h"

namespace bleHaptics
{
    BLECharacteristic *pVibrationCharacteristic;
    class VibrationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto value = pCharacteristic->getValue();

            auto data = value.data();
            auto length = value.length();

            haptics::vibrate((uint8_t *)data, length);
        }
    };

    void setup()
    {
        pVibrationCharacteristic = ble::createCharacteristic(GENERATE_UUID("d000"), NIMBLE_PROPERTY::WRITE, "haptics vibration");
        pVibrationCharacteristic->setCallbacks(new VibrationCharacteristicCallbacks());
    }

    void loop()
    {
        
    }
} // namespace bleHaptics
