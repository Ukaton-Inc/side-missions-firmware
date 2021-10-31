#include <Arduino.h>

#include "definitions.h"
#include "esp_pm.h"
#include "battery.h"
#include "eepromUtils.h"
#include "name.h"
#include "motion.h"
#include "pressure.h"
#include "wifiServer.h"
#include "powerManagement.h"

void setup()
{
    Serial.begin(115200);
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);

#if DEBUG
    Serial.println("setup");
#endif

    eepromUtils::setup();
    name::setup();
    motion::setup();
#if IS_INSOLE
    pressure::setup();
#endif
    wifiServer::setup();
#if ENABLE_POWER_MANAGEMENT
    powerManagement::setup();
#endif
}

void loop()
{
    motion::loop();
    wifiServer::loop();
#if ENABLE_POWER_MANAGEMENT
    powerManagement::loop();
#endif
}