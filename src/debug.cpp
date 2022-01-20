#include "debug.h"
#include <Preferences.h>
#include "definitions.h"

namespace debug
{
    bool isEnabled = DEBUG;

    Preferences preferences;
    void setup()
    {
        preferences.begin("debug");
        isEnabled = preferences.getBool("isEnabled", isEnabled);
    }

    bool getEnabled() {
        return isEnabled;
    }
    void setEnabled(bool _isEnabled)
    {
        isEnabled = _isEnabled;
        preferences.putBool("isEnabled", isEnabled);
        Serial.print("changed isEnabled to: ");
        Serial.println(isEnabled);
    }
} // namespace debug