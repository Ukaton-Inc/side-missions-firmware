#include "bleMotionCalibration.h"
#include "sensor/motionSensor.h"
#include "wifi/webSocket.h"

namespace bleMotionCalibration {
    BLECharacteristic *pCharacteristic;

    void setup() {
        pCharacteristic = ble::createCharacteristic(GENERATE_UUID("5001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "motion calibration");
    }
    void updateDataCharacteristic(bool notify = false) {
        pCharacteristic->setValue(motionSensor::calibration, sizeof(motionSensor::calibration));
        if (notify) {
            pCharacteristic->notify();
        }
    }

    unsigned long lastCalibrationUpdateTime;
    void loop() {
        if (pCharacteristic->getSubscribedCount() > 0 && lastCalibrationUpdateTime != motionSensor::lastCalibrationUpdateTime && !webSocket::isConnectedToClient()) {
            lastCalibrationUpdateTime = motionSensor::lastCalibrationUpdateTime;
            updateDataCharacteristic(true);
        }
    }
} // namespace bleMotionCalibration