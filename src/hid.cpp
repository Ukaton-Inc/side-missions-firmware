#include "hid.h"
#include "motion.h"

namespace hid
{
    BleGamepad bleGamepad("Ukaton Motion Module", "Ukaton", 100);

    void setup()
    {
        bleGamepad.setAutoReport(false);
        bleGamepad.setControllerType(CONTROLLER_TYPE_JOYSTICK);
        bleGamepad.begin(0, 0, true, true, false, false, false, false, false, false, false, false, false, false, false);
        bleGamepad.setLeftThumb(0, 0);
    }

    bool wasConnected = false;
    void loop()
    {
        bool isConnected = bleGamepad.isConnected();
        if (wasConnected != bleGamepad.isConnected()) {
            if (isConnected) {
                Serial.println("connected");
                motion::start();
            }
            else {
                Serial.println("disconnected");
                motion::stop();
            }
            wasConnected = isConnected;
        }
    }
} // namespace hid