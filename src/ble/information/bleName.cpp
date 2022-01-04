#include "bleName.h"
#include "information/name.h"

namespace bleName {
    BLECharacteristic *pCharacteristic;
    class CharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto newName = pCharacteristic->getValue();
            name::setName((char *) newName.c_str());
            
            auto updatedName = *name::getName();
            pCharacteristic->setValue(updatedName);
        }
    };

    const std::string *getName() {
        return name::getName();
    }

    void setup()
    {
        pCharacteristic = ble::createCharacteristic(GENERATE_UUID("4001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "name");
        pCharacteristic->setCallbacks(new CharacteristicCallbacks());
        pCharacteristic->setValue(*name::getName());
    }
} // namespace bleName