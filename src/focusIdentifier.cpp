#include "focusIdentifier.h"

namespace focusIdentifier
{
    uint8_t focusIdentifier[4] = {0};
    BLECharacteristic *pFocusIdentifierCharacteristic;

    void setup()
    {
        pFocusIdentifierCharacteristic = ble::createCharacteristic("B30910E9-61F2-49B1-96B0-54F6409CD17D", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY, "Focus Identifier");
        pFocusIdentifierCharacteristic->setValue(focusIdentifier, sizeof(focusIdentifier));
    }
} // namespace focusIdentifier