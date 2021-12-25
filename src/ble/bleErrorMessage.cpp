#include "bleErrorMessage.h"
#include "errorMessage.h"

namespace bleErrorMessage {
    BLECharacteristic *pErrorMessageCharacteristic;

    void setup()
    {
        pErrorMessageCharacteristic = ble::createCharacteristic(GENERATE_UUID("2001"), NIMBLE_PROPERTY::NOTIFY , "error message");
    }

    void send(const char* message) {
        pErrorMessageCharacteristic->setValue(message);
        pErrorMessageCharacteristic->notify();
    }
} // namespace bleErrorMessage