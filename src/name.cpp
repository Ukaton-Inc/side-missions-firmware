#include "name.h"
#include "ble.h"
#include "eepromUtils.h"

namespace name
{
    std::string name = DEFAULT_BLE_NAME;
    uint16_t eepromAddress;

    void loadFromEEPROM() {
        name = EEPROM.readString(eepromAddress).c_str();
    }
    void saveToEEPROM() {
        EEPROM.writeString(eepromAddress, name.substr(0, min((int)name.length(), MAX_NAME_LENGTH)).c_str());
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

        pCharacteristic = ble::createCharacteristic(GENERATE_UUID("1000"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "name");
        pCharacteristic->setCallbacks(new CharacteristicCallbacks());
        pCharacteristic->setValue(name);
        ::esp_ble_gap_set_device_name(name.c_str());
    }

    void setName(char *newName)
    {
        if (strlen(newName) <= MAX_NAME_LENGTH) {
            name = newName;
            ::esp_ble_gap_set_device_name(name.c_str());
            saveToEEPROM();
        }
        else {
            log_e("name's too long");
        }
    }
} // namespace name