#include <Arduino.h>

#include "eepromUtils.h"
#include "ble.h"
#include "name.h"
#include "battery.h"
#include "imu.h"
#include "tinyML.h"
#include "fileTransfer.h"

void setup()
{
    Serial.begin(115200);
    
    eepromUtils::setup();
    ble::setup();
    name::setup();
    battery::setup();
    imu::setup();
    tinyML::setup();
    fileTransfer::setup();

    ble::start();
}

void loop()
{
    battery::loop();
    imu::loop();
}