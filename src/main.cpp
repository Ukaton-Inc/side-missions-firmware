#include <Arduino.h>

#include "eepromUtils.h"
#include "ble.h"
#include "name.h"
#include "battery.h"
#include "motion.h"
#include "tfLite.h"
#include "fileTransfer.h"

void setup()
{
    Serial.begin(115200);
    
    eepromUtils::setup();
    ble::setup();
    name::setup();
    battery::setup();
    motion::setup();
    tfLite::setup();
    fileTransfer::setup();

    ble::start();
}

void loop()
{
    battery::loop();
    motion::loop();
    tfLite::loop();
}