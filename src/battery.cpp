#include "battery.h"
#include "ble.h"

namespace battery {
    const uint16_t battery_level_delay_ms = 1000;
    uint8_t batteryLevel;

    BLEService *pBatteryService;

    BLECharacteristic *pBatteryLevelCharacteristic;
    uint8_t batteryLevelCharacteristicValue[1];

    uint8_t getBatteryLevel() {
        return 100;
    }
    void updateBatteryLevel() {
        batteryLevel = getBatteryLevel();
        if (ble::isServerConnected) {
            batteryLevelCharacteristicValue[0] = batteryLevel;
            pBatteryLevelCharacteristic->setValue((uint8_t *) batteryLevelCharacteristicValue, sizeof(batteryLevelCharacteristicValue));
            pBatteryLevelCharacteristic->notify();
        }
    }

    unsigned long currentTime;
    unsigned long lastTime = 0;

    void setup() {
        pBatteryService = ble::pServer->createService(BLEUUID((uint16_t)0x180F));
        pBatteryLevelCharacteristic = ble::createCharacteristic(BLEUUID((uint16_t)0x2A19).toString().c_str(), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "Battery Level", pBatteryService);
        pBatteryService->start();
    }
    void loop() {
        currentTime = millis();
        if (ble::isServerConnected && currentTime >= lastTime + battery_level_delay_ms) {
            lastTime = currentTime;
            updateBatteryLevel();
        }
    }
} // namespace battery