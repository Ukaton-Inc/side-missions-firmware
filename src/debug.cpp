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
        preferences.end();
    }

    bool getEnabled() {
        return isEnabled;
    }
    void setEnabled(bool _isEnabled)
    {
        isEnabled = _isEnabled;

        preferences.begin("debug");
        preferences.putBool("isEnabled", isEnabled);
        preferences.end();
        
        Serial.print("changed isEnabled to: ");
        Serial.println(isEnabled);
    }
} // namespace debug