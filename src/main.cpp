#include <Arduino.h>

#include "eepromUtils.h"
#include "ble.h"
#include "deviceInformation.h"
#include "wearableDevice.h"
#include "sensors.h"
#include "gestures.h"
#include "focusIdentifier.h"
//#include "bmapService.h"

void setup()
{
    Serial.begin(115200);

    eepromUtils::setup();
    ble::setup();
    deviceInformation::setup();
    wearableDevice::setup();
    sensors::setup();
    gestures::setup();
    focusIdentifier::setup();
    //bmapService::setup();

    deviceInformation::start();
    //bmapService::start();
    ble::start();
}

void loop()
{
    sensors::loop();
    gestures::loop();
}