#include <Arduino.h>

#include "definitions.h"
#include "ble/ble.h"
#include "wifi/wifi.h"
#include "information/name.h"
#include "battery.h"
#include "sensor/motionSensor.h"
#include "information/type.h"
#include "moveToWake.h"
#include "sensor/pressureSensor.h"
#include "sensor/sensorData.h"
#include "weight/weightData.h"
#include "steps.h"
#include "haptics.h"

void setup()
{
    Serial.begin(115200);
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);
    
    battery::setup();
    name::setup();
    type::setup();
    motionSensor::setup();
    moveToWake::setup();
    pressureSensor::setup();
    haptics::setup();
    steps::setup();
    wifi::setup();
    ble::setup();
}

void loop()
{
    battery::loop();
    motionSensor::loop();
    moveToWake::loop();
    sensorData::loop();
    steps::loop();
    haptics::loop();
    weightData::loop();
    wifi::loop();
    ble::loop();
}