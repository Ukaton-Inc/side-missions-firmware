#include "errorMessage.h"
#include "eepromUtils.h"
#include "ble/bleErrorMessage.h"

namespace errorMessage
{
    void send(const char* message) {
        bleErrorMessage::send(message);
    }
} // namespace errorMessage