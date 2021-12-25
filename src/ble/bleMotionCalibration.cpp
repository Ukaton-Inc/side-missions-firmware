#include "bleMotionCalibration.h"
#include "motionSensor.h"

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
        if (lastCalibrationUpdateTime != motionSensor::lastCalibrationUpdateTime) {
            lastCalibrationUpdateTime = motionSensor::lastCalibrationUpdateTime;
            updateDataCharacteristic(true);
        }
    }
} // namespace bleMotionCalibration