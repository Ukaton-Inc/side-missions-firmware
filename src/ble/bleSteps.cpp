#include "bleSteps.h"
#include "steps.h"

namespace bleSteps
{
    BLECharacteristic *pIsTrackingCharacteristic;
    void updateIsTrackingCharacteristic()
    {
        pIsTrackingCharacteristic->setValue(steps::isTracking());
    }
    class IsTrackingCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue().data();
            auto isTracking = (bool) data[0];
            if (isTracking) {
                steps::startTracking();
            }
            else {
                steps::stopTracking();
            }
            updateIsTrackingCharacteristic();
        }
    };

    BLECharacteristic *pMassThresholdCharacteristic;
    void updateMassThresholdCharacteristic() {
        pMassThresholdCharacteristic->setValue(steps::getMassThreshold());
    }
    class MassThresholdCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue();
            auto rawData = (float *) data.c_str();
            auto massThreshold = rawData[0];
            steps::setMassThreshold(massThreshold);
            updateMassThresholdCharacteristic();
        }
    };

    BLECharacteristic *pDataCharacteristic;
    unsigned long lastDataUpdateTime;
    void updateDataCharacteristic(bool notify = true)
    {
        pDataCharacteristic->setValue(steps::getSteps());
        //Serial.println(steps::getSteps());
        if (notify && pDataCharacteristic->getSubscribedCount() > 0) {
            pDataCharacteristic->notify();
        }
    }
    class DataCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue();
            auto rawData = (uint32_t *) data.c_str();
            auto newSteps = rawData[0];
            steps::setSteps(newSteps);
            updateDataCharacteristic(false);
        }
    };

    void setup()
    {
        pIsTrackingCharacteristic = ble::createCharacteristic(GENERATE_UUID("c000"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "is tracking steps");
        pIsTrackingCharacteristic->setCallbacks(new IsTrackingCharacteristicCallbacks());
        updateIsTrackingCharacteristic();

        pMassThresholdCharacteristic = ble::createCharacteristic(GENERATE_UUID("c001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "step data mass threshold");
        pMassThresholdCharacteristic->setCallbacks(new MassThresholdCharacteristicCallbacks());
        updateMassThresholdCharacteristic();

        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("c002"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY | NIMBLE_PROPERTY::WRITE, "step data");
        pDataCharacteristic->setCallbacks(new DataCharacteristicCallbacks());
        updateDataCharacteristic();
    }

    void loop()
    {
        if (lastDataUpdateTime != steps::lastTimeStepsWereUpdated)
        {
            lastDataUpdateTime = steps::lastTimeStepsWereUpdated;
            updateDataCharacteristic();
        }
    }
} // namespace bleSteps
