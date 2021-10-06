#include "eepromUtils.h"

namespace eepromUtils
{
    const uint16_t MAX_SIZE = 512;
    const uint8_t SCHEMA = 0;

    unsigned char schema;
    bool firstInitialized = false;
    uint16_t freeAddress = 1;

    void setup()
    {
        EEPROM.begin(MAX_SIZE);

        schema = EEPROM.readUChar(0);
        firstInitialized = (schema != SCHEMA);
        if (firstInitialized)
        {
            schema = SCHEMA;
            EEPROM.writeUChar(0, schema);
            EEPROM.commit();
        }
    }

    uint16_t getAvailableSpace() {
        return MAX_SIZE - freeAddress;
    }

    uint16_t reserveSpace(uint16_t size)
    {
        if (size <= getAvailableSpace()) {
            uint16_t address = freeAddress;
            freeAddress += size;
            return address;
        }
        else {
            log_e("not enough space in EEPROM");
        }
    }

} // namespace eepromUtils