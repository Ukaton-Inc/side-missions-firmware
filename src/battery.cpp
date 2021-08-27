#include "battery.h"

namespace battery {
    const uint16_t battery_level_delay_ms = 1000;
    uint8_t batteryLevel;

    uint8_t getBatteryLevel() {
        return 100;
    }
    void updateBatteryLevel() {
        batteryLevel = getBatteryLevel();
    }

    unsigned long currentTime;
    unsigned long lastTime = 0;

    void setup() {
        
    }
    void loop() {
        currentTime = millis();
        if (currentTime >= lastTime + battery_level_delay_ms) {
            lastTime = currentTime;
            updateBatteryLevel();
        }
    }
} // namespace battery