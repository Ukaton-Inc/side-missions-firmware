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

void setup()
{
    Serial.begin(115200);
    setCpuFrequencyMhz(CPU_FREQUENCY_MHZ);

#if DEBUG
    Serial.println("setup");
#endif

    name::setup();
    type::setup();
    motionSensor::setup();
    moveToWake::setup();
    pressureSensor::setup();
    wifi::setup();
    ble::setup();
    steps::setup();
}

void loop()
{
    motionSensor::loop();
    moveToWake::loop();
    sensorData::loop();
    weightData::loop();
    wifi::loop();
    ble::loop();
    steps::loop();
}