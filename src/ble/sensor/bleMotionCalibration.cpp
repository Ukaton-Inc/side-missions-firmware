#include "bleMotionCalibration.h"
#include "sensor/motionSensor.h"
#include "wifi/webSocket.h"

namespace bleMotionCalibration {
    BLECharacteristic *pCharacteristic;
    bool isSubscribed = false;
    class CharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
            if (subValue == 0) {
                isSubscribed = false;
            }
            else if (subValue == 1) {
                isSubscribed = true;
            }
        }
    };

    void setup() {
        pCharacteristic = ble::createCharacteristic(GENERATE_UUID("5001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "motion calibration");
        pCharacteristic->setCallbacks(new CharacteristicCallbacks());
    }
    void updateDataCharacteristic(bool notify = false) {
        pCharacteristic->setValue(motionSensor::calibration, sizeof(motionSensor::calibration));
        if (notify) {
            pCharacteristic->notify();
        }
    }

    unsigned long lastCalibrationUpdateTime;
    void loop() {
        if (isSubscribed && lastCalibrationUpdateTime != motionSensor::lastCalibrationUpdateTime && !webSocket::isConnectedToClient()) {
            lastCalibrationUpdateTime = motionSensor::lastCalibrationUpdateTime;
            updateDataCharacteristic(true);
        }
    }
} // namespace bleMotionCalibration