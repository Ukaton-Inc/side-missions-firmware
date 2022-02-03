#include "name.h"
#include <Preferences.h>
#include "definitions.h"
#include "services/gap/ble_svc_gap.h"
#include <WiFi.h>

namespace name
{
    std::string name = DEFAUlT_NAME;

    Preferences preferences;
    void setup()
    {
        preferences.begin("name");
        if (preferences.isKey("name")) {
            name = preferences.getString("name").c_str();
        }

        Serial.print("name: ");
        Serial.println(name.c_str());

        ble_svc_gap_device_name_set(name.c_str());
        WiFi.setHostname(name.c_str());
    }

    const std::string *getName() {
        return &name;
    }
    void setName(const char *newName, size_t length)
    {
        if (length <= MAX_NAME_LENGTH) {
            name.assign(newName, length);
            preferences.putString("name", name.c_str());
            Serial.print("changed name to: ");
            Serial.println(name.c_str());
            ble_svc_gap_device_name_set(name.c_str());
            WiFi.setHostname(name.c_str());
        }
        else {
            log_e("name's too long");
        }
    }
    void setName(const char *newName)
    {
        uint8_t length = strlen(newName);
        setName(newName, length);
    }
} // namespace name