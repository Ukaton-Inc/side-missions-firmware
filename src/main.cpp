#include <Arduino.h>

#include "definitions.h"
#include "esp_pm.h"
#include "battery.h"
#include "eepromUtils.h"
#include "name.h"
#include "motion.h"
#include "wifiServer.h"

void setup()
{
    Serial.begin(115200);
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);

    eepromUtils::setup();
    name::setup();
    motion::setup();
    wifiServer::setup();
}

void loop()
{
    motion::loop();
    wifiServer::loop();
}