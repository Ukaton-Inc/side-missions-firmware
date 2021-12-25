#include "bleDebug.h"
#include "debug.h"

namespace bleDebug {
    BLECharacteristic *pCharacteristic;
    class CharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue().data();
            bool isEnabled = data[0];
            debug::setEnabled(isEnabled);
        }
    };

    bool getEnabled() {
        return debug::getEnabled();
    }

    void setup()
    {
        pCharacteristic = ble::createCharacteristic(GENERATE_UUID("1001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "debug");
        pCharacteristic->setCallbacks(new CharacteristicCallbacks());
        pCharacteristic->setValue(debug::getEnabled());
    }
} // namespace bleDebug