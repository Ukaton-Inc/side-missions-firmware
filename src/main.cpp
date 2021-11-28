#include <Arduino.h>

#include "definitions.h"
#include "eepromUtils.h"
#include "ble.h"
#include "name.h"
#include "battery.h"
#include "motion.h"
#include "tfLite.h"
#if IS_INSOLE
#include "pressure.h"
#include "weight.h"
#endif
#include "powerManagement.h"
#include "fileTransfer.h"

void setup()
{
    Serial.begin(115200);

    eepromUtils::setup();
    ble::setup();
    name::setup();
    battery::setup();
    motion::setup();
    fileTransfer::setup();
#if IS_INSOLE
    pressure::setup();
    weight::setup();
#endif
    powerManagement::setup();
    tfLite::setup();
    ble::start();
}

void loop()
{
    battery::loop();
    motion::loop();
#if IS_INSOLE
    pressure::loop();
    weight::loop();
#endif
    powerManagement::loop();
    tfLite::loop();
}