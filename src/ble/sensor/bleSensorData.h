#pragma once
#ifndef _BLE_SENSOR_DATA_
#define _BLE_SENSOR_DATA_

#include "ble/ble.h"

namespace bleSensorData
{
    extern BLECharacteristic *pConfigurationCharacteristic;
    class ConfigurationCharacteristicCallbacks;
    extern BLECharacteristic *pDataCharacteristic;

    void updateConfigurationCharacteristic();
    void clearConfigurations();

    void setup();
    void loop();
} // namespace bleSensorData

#endif // _BLE_SENSOR_DATA_