#include <Arduino.h>
#include <Wire.h>

#include "definitions.h"
#include "ble/ble.h"
#include "wifi/wifi.h"
#include "information/name.h"
#include "battery.h"
#include "sensor/motionSensor.h"
#include "information/type.h"
#include "powerManagement.h"
#include "sensor/pressureSensor.h"
#include "sensor/sensorData.h"
#include "weight/weightData.h"
#include "steps.h"
#include "haptics.h"

#include <Preferences.h>

#define TEST_BATTERY true

#if TEST_BATTERY
bool recordBatteryLevels = false;
Preferences preferences;
unsigned long lastUpdateBatteryLevelTime = 0;
unsigned long batteryLevels[100]{0};
char batteryLevelCharBuffer[3];
void formatBatteryLevelCharBuffer(uint8_t batteryLevel)
{
    snprintf(batteryLevelCharBuffer, 12+3, "batteryLevel%u", batteryLevel);
    Serial.println(batteryLevelCharBuffer);
}
void updatePreferences(uint8_t batteryLevel) {
    batteryLevels[batteryLevel] = millis();
    formatBatteryLevelCharBuffer(batteryLevel);
    Serial.printf("battery level changed to %u%% at %ums\n", batteryLevel, batteryLevels[batteryLevel]);
    preferences.begin("batteryTest");
    preferences.putULong(batteryLevelCharBuffer, batteryLevels[batteryLevel]);
    preferences.end();
}
#endif

void setup()
{
    Serial.begin(115200);
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);
    Wire.begin();

    battery::setup();
    name::setup();
    type::setup();
    motionSensor::setup();
    powerManagement::setup();
    pressureSensor::setup();
    haptics::setup();
    steps::setup();
    // wifi::setup();
    // ble::setup();

#if TEST_BATTERY
    preferences.begin("batteryTest");
    if (!preferences.isKey("started"))
    {
        recordBatteryLevels = true;
        preferences.putBool("started", true);
    }
    else
    {
        recordBatteryLevels = false;
    }
    preferences.end();
    
    Serial.printf("recording battery levels? %u\n", recordBatteryLevels);
    delay(1000);
    for (uint8_t i = 0; i < 100; i++) {
        //updatePreferences(i);
    }
    if (!recordBatteryLevels)
    {
        preferences.begin("batteryTest");
        for (uint8_t batteryLevel = 0; batteryLevel < 100; batteryLevel++)
        {
            formatBatteryLevelCharBuffer(batteryLevel);
            Serial.printf("time when battery level was at %u%%: %ums\n", batteryLevel, preferences.getULong(batteryLevelCharBuffer, 0));
        }
        preferences.end();
    }
#endif
}

void loop()
{
    battery::loop();
    motionSensor::loop();
    powerManagement::loop();
    sensorData::loop();
    steps::loop();
    haptics::loop();
    weightData::loop();
    // wifi::loop();
    // ble::loop();

#if TEST_BATTERY
    if (recordBatteryLevels && lastUpdateBatteryLevelTime != battery::lastUpdateBatteryLevelTime)
    {
        auto batteryLevel = (uint8_t)battery::getLevel();
        Serial.println(batteryLevel);
        if (batteryLevel >= 0 && batteryLevel < 100 && (batteryLevels[batteryLevel] == 0))
        {
            updatePreferences(batteryLevel);
        }
        lastUpdateBatteryLevelTime = battery::lastUpdateBatteryLevelTime;
    }
#endif
}