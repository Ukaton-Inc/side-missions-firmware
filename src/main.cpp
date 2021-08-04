#include <Arduino.h>

#include "definitions.h"
#include "eepromUtils.h"
#include "ble.h"
#include "name.h"
#include "battery.h"
#if IS_INSOLE
    #include "crappyMotion.h"
    #include "pressure.h"
#else
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
        crappyMotion::setup();
    #else
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
    #else
        motion::loop();
        tfLite::loop();
    #endif
}