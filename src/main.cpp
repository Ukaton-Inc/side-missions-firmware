#include <Arduino.h>

#include "eepromUtils.h"
#include "name.h"
#include "battery.h"
#include "motion.h"
#include "pressure.h"
#include "websocketServer.h"

void setup()
{
    Serial.begin(115200);

    eepromUtils::setup();
    name::setup();    
    battery::setup();
    motion::setup();    
    pressure::setup();    
    websocketServer::setup();
}

void loop()
{
    battery::loop();
    motion::loop();
    pressure::loop();
    websocketServer::loop();
}