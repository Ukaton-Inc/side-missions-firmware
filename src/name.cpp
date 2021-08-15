#include "name.h"
#include "ble.h"
#include "eepromUtils.h"
#include "services/gap/ble_svc_gap.h"

namespace name
{
    const uint8_t MAX_NAME_LENGTH = 30;

    std::string name = DEFAULT_BLE_NAME;
    uint16_t eepromAddress;

    void loadFromEEPROM() {
        name = EEPROM.readString(eepromAddress).c_str();
    }
    void saveToEEPROM() {
        EEPROM.writeString(eepromAddress, name.substr(0, min((const uint8_t)name.length(), MAX_NAME_LENGTH)).c_str());
        EEPROM.commit();
    }

    BLECharacteristic *pCharacteristic;
    class CharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            std::string newName = pCharacteristic->getValue();
            setName((char *) newName.c_str());
        }
    };

    void setup()
    {
        eepromAddress = eepromUtils::reserveSpace(MAX_NAME_LENGTH);

        if (eepromUtils::firstInitialized) {
            saveToEEPROM();
        }
        else {
            loadFromEEPROM();
        }

        pCharacteristic = ble::createCharacteristic(GENERATE_UUID("1000"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "name");
        pCharacteristic->setCallbacks(new CharacteristicCallbacks());
        pCharacteristic->setValue(name);
        ble_svc_gap_device_name_set(name.c_str());
    }

    void setName(char *newName)
    {
        if (strlen(newName) <= MAX_NAME_LENGTH) {
            name = newName;
            ble_svc_gap_device_name_set(name.c_str());
            saveToEEPROM();
        }
        else {
            log_e("name's too long");
        }
    }
} // namespace name