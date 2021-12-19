#include <Arduino.h>

#include "definitions.h"
#include "eepromUtils.h"
#include "ble/ble.h"
#include "name.h"
#include "battery.h"
#include "motionSensor.h"
#include "type.h"
#include "moveToWake.h"
#include "pressureSensor.h"
#include "sensorData.h"

void setup()
{
    Serial.begin(115200);
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);

#if DEBUG
    Serial.println("setup");
#endif

    eepromUtils::setup();
    name::setup();
    type::setup();
    motionSensor::setup();
    moveToWake::setup();
    pressureSensor::setup();
    ble::setup();
}

void loop()
{
    motionSensor::loop();
    moveToWake::loop();
    sensorData::loop();
    ble::loop();
}