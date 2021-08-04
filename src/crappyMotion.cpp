#include "crappyMotion.h"
#include "ble.h"
#include <MPU9250.h>

namespace crappyMotion
{
    MPU9250 mpu;

    const uint16_t data_delay_ms = 20;
    const uint8_t data_base_offset = sizeof(uint8_t) + sizeof(unsigned long);

    BLECharacteristic *pCalibrateCharacteristic;
    BLECharacteristic *pCalibrationStatusCharacteristic;
    BLECharacteristic *pCalibrationCharacteristic;
    bool shouldCalibrate = false;
    class CalibrateCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t *characteristicData = pCharacteristic->getData();
            shouldCalibrate = characteristicData[0] == 1;
        }
    };
    bool isCalibrating = false;
    void calibrate()
    {
        isCalibrating = true;

        uint8_t data[1] = {0};

        Serial.println("Accel Gyro calibration will start in 5sec.");
        Serial.println("Please leave the device still on the flat plane.");
        data[0] = CALIBRATING_ACCELEROMETER_AND_GYROSCOPE;
        pCalibrationStatusCharacteristic->setValue(data, sizeof(data));
        pCalibrationStatusCharacteristic->notify();
        delay(5000);
        mpu.calibrateAccelGyro();

        Serial.println("Mag calibration will start in 5sec.");
        Serial.println("Please Wave device in a figure eight until done.");
        data[0] = CALIBRATING_MAGNETOMETER;
        pCalibrationStatusCharacteristic->setValue(data, sizeof(data));
        pCalibrationStatusCharacteristic->notify();
        delay(5000);
        mpu.calibrateMag();

        Serial.println("Calibration Complete!");
        data[0] = DONE_CALIBRATING;
        pCalibrationStatusCharacteristic->setValue(data, sizeof(data));
        pCalibrationStatusCharacteristic->notify();

        isCalibrating = false;
    }

    uint16_t delays[NUMBER_OF_DATA_TYPES] = {0};
    BLECharacteristic *pConfigurationCharacteristic;
    class ConfigurationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t length = pCharacteristic->m_value.getLength();
            if (length == NUMBER_OF_DATA_TYPES * sizeof(uint16_t))
            {
                uint16_t *characteristicData = (uint16_t *)pCharacteristic->getData();
                for (int i = 0; i < NUMBER_OF_DATA_TYPES; i++)
                {
                    delays[i] = characteristicData[i];
                    delays[i] -= delays[i] % data_delay_ms;
                }
            }
            pCharacteristic->setValue((uint8_t *)delays, sizeof(delays));
        }
    };

    BLECharacteristic *pDataCharacteristic;
    uint8_t dataCharacteristicValue[ble::max_characteristic_value_length];
    uint8_t dataCharacteristicValueBitmask;
    uint8_t dataCharacteristicValueOffset;
    uint8_t dataCharacteristicValueBuffer[ble::max_characteristic_value_length];
    int16_t rawData[7];
    unsigned long lastDataLoopTime;
    void updateData()
    {
        dataCharacteristicValueBitmask = 0;
        dataCharacteristicValueOffset = data_base_offset;
        for (uint8_t i = 0; i < NUMBER_OF_DATA_TYPES; i++)
        {
            if (delays[i] != 0 && lastDataLoopTime % delays[i] == 0)
            {
                addData(i, i);
            }
        }
        if (dataCharacteristicValueBitmask != 0)
        {
            sendData();
        }
    }
    void addData(uint8_t dataType, uint8_t bitIndex)
    {
        uint8_t size = 0;
        switch (dataType)
        {
        case ACCELERATION:
            size = sizeof(int16_t) * 3;
            MEMCPY(&dataCharacteristicValueBuffer, &rawData[0], size);
            break;
        case ROTATION_RATE:
            size = sizeof(int16_t) * 3;
            MEMCPY(&dataCharacteristicValueBuffer, &rawData[4], size);
            break;
        case MAGNETOMETER:
            size = sizeof(mpu.m);
            MEMCPY(&dataCharacteristicValueBuffer, &mpu.m, size);
            break;
        case QUATERNION:
            size = sizeof(mpu.q);
            MEMCPY(&dataCharacteristicValueBuffer, &mpu.q, size);
            break;
        }
        if (dataCharacteristicValueOffset + size > ble::max_characteristic_value_length)
        {
            sendData();
        }

        bitSet(dataCharacteristicValueBitmask, bitIndex);

        MEMCPY(&dataCharacteristicValue[dataCharacteristicValueOffset], &dataCharacteristicValueBuffer, size);
        dataCharacteristicValueOffset += size;
    }
    void sendData()
    {
        MEMCPY(&dataCharacteristicValue[0], &dataCharacteristicValueBitmask, 1);
        MEMCPY(&dataCharacteristicValue[1], &currentTime, sizeof(currentTime));
        pDataCharacteristic->setValue((uint8_t *)dataCharacteristicValue, dataCharacteristicValueOffset);
        pDataCharacteristic->notify();

        memset(&dataCharacteristicValue, 0, sizeof(dataCharacteristicValue));
        dataCharacteristicValueBitmask = 0;
        dataCharacteristicValueOffset = data_base_offset;
    }
    bool hasAtLeastOneNonzeroDelay()
    {
        bool _hasAtLeastOneNonzeroDelay = false;
        for (uint8_t i = 0; i < NUMBER_OF_DATA_TYPES && !_hasAtLeastOneNonzeroDelay; i++)
        {
            if (delays[i] != 0)
            {
                _hasAtLeastOneNonzeroDelay = true;
            }
        }
        return _hasAtLeastOneNonzeroDelay;
    }
    void dataLoop()
    {
        if (!isCalibrating && hasAtLeastOneNonzeroDelay() && currentTime >= lastDataLoopTime + data_delay_ms)
        {
            if (mpu.update())
            {
                mpu.read_accel_gyro(rawData);
                updateData();
            }
            lastDataLoopTime = currentTime - (currentTime % data_delay_ms);
        }
    }

    void setup()
    {
        Wire.begin();
        delay(2000);
        if (!mpu.setup(0x68))
        {
            Serial.println("MPU connection failed. Please check your connection with `connection_check` example.");
        }

        pCalibrateCharacteristic = ble::createCharacteristic(GENERATE_UUID("2000"), BLECharacteristic::PROPERTY_WRITE, "calibrate imu");
        pCalibrateCharacteristic->setCallbacks(new CalibrateCharacteristicCallbacks());

        pCalibrationStatusCharacteristic = ble::createCharacteristic(GENERATE_UUID("2001"), BLECharacteristic::PROPERTY_NOTIFY, "imu calibration status");

        pConfigurationCharacteristic = ble::createCharacteristic(GENERATE_UUID("2002"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "imu configuration");
        pConfigurationCharacteristic->setValue((uint8_t *)delays, sizeof(delays));
        pConfigurationCharacteristic->setCallbacks(new ConfigurationCharacteristicCallbacks());

        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("2003"), BLECharacteristic::PROPERTY_NOTIFY, "imu data");
    }
    void stop() {
        memset(&delays, 0, sizeof(delays));
        pConfigurationCharacteristic->setValue((uint8_t *) delays, sizeof(delays));
    }

    unsigned long currentTime;
    void loop()
    {
        currentTime = millis();

        if (ble::isServerConnected)
        {
            if (shouldCalibrate) {
                shouldCalibrate = false;
                calibrate();
            }
            else {
                dataLoop();
            }
        }
    }
} // namespace crappyMotion