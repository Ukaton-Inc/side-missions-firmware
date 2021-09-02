#include <Arduino.h>

#include "definitions.h"
#include "eepromUtils.h"
#include "ble.h"
#include "name.h"
#include "battery.h"
#if IS_INSOLE
    #include "crappyMotion.h"
    #include "pressure.h"
    #include "weight.h"
#else
    #include "powerManagement.h"
    #include "motion.h"
    #include "tfLite.h"
    #include "fileTransfer.h"
#endif

void setup()
{
    Serial.begin(115200);

    eepromUtils::setup();
    ble::setup();
    name::setup();
    battery::setup();
    #if IS_INSOLE
        pressure::setup();
        weight::setup();
        crappyMotion::setup();
    #else
        powerManagement::setup();
        motion::setup();
        tfLite::setup();
        fileTransfer::setup();
    #endif
    
    ble::start();
}

void loop()
{
    battery::loop();
    #if IS_INSOLE
        pressure::loop();
        crappyMotion::loop();
        weight::loop();
    #else
        powerManagement::loop();
        motion::loop();
        tfLite::loop();
    #endif
}