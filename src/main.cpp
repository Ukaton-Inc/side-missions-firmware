#include <Arduino.h>

#include "eepromUtils.h"
#include "motion.h"
#include "hid.h"

void setup()
{
    Serial.begin(115200);

    eepromUtils::setup();
    motion::setup();    
    hid::setup();
}

void loop()
{
    hid::loop();
    motion::loop();
}