#include "bleType.h"
#include "information/type.h"

namespace bleType {
    BLECharacteristic *pCharacteristic;
    class CharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue().data();
            auto type = (type::Type) data[0];
            type::setType(type);
        }
    };

    void setup()
    {
        pCharacteristic = ble::createCharacteristic(GENERATE_UUID("3001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "device type");
        pCharacteristic->setCallbacks(new CharacteristicCallbacks());
        pCharacteristic->setValue(type::getType());
    }
} // namespace bleType