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
    motion::setup();
    pressure::setup();
    wifiServer::setup();
    powerManagement::setup();
}

void loop()
{
    motion::loop();
    wifiServer::loop();
    powerManagement::loop();
}