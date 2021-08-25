#include <Arduino.h>

#include "eepromUtils.h"
#include "motion.h"
#include "websocketServer.h"

void setup()
{
    Serial.begin(115200);

    eepromUtils::setup();
    motion::setup();    
    websocketServer::setup();
}

void loop()
{
    motion::loop();
    websocketServer::loop();
}