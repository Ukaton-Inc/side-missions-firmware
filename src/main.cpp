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

    eepromUtils::setup();
    name::setup();
#if !IS_INSOLE
    motion::setup();
#endif
#if IS_INSOLE
    pressure::setup();
#endif
    wifiServer::setup();
#if !IS_INSOLE
    powerManagement::setup();
#endif
}

void loop()
{
#if !IS_INSOLE
    motion::loop();
#endif
    wifiServer::loop();
#if !IS_INSOLE
    powerManagement::loop();
#endif
}