#include "imu.h"
#include "ble.h"

namespace imu {
    Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);
    bool isBnoAwake = false;

    BLECharacteristic *pCalibrationCharacteristic;
    uint8_t callibration[NUMBER_OF_CALIBRATION_TYPES];
    void updateCalibration() {
        bno.getCalibration(&callibration[0], &callibration[1], &callibration[2], &callibration[3]);
        pCalibrationCharacteristic->setValue((uint8_t *) callibration, sizeof(callibration));
        pCalibrationCharacteristic->notify();
    }
    unsigned long lastCalibrationLoopTime = 0;
    void calibrationLoop() {
        if (currentTime >= lastCalibrationLoopTime + IMU_CALIBRATION_DELAY_MS) {
            lastCalibrationLoopTime += IMU_CALIBRATION_DELAY_MS;
            updateCalibration();
        }
    }

    uint16_t delays[NUMBER_OF_DATA_TYPES] = {0};
    BLECharacteristic *pConfigurationCharacteristic;
    class ConfigurationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t length = pCharacteristic->m_value.getLength();
            if (length == 6 * sizeof(uint16_t)) {
                uint16_t *characteristicData = (uint16_t *) pCharacteristic->getData();
                for (int i = 0; i < NUMBER_OF_DATA_TYPES; i++) {
                    delays[i] = characteristicData[i];
                    delays[i] -= delays[i] % IMU_DATA_DELAY_MS;
                }
            }
            pCharacteristic->setValue((uint8_t *) delays, sizeof(delays));
        }
    };

    BLECharacteristic *pDataCharacteristic;
    uint8_t dataCharacteristicValue[MAX_CHARACTERISTIC_VALUE_LENGTH];
    uint8_t dataCharacteristicValueBitmask;
    uint8_t dataCharacteristicValueOffset;
    uint8_t dataCharacteristicValueBuffer[8] = {0};
    unsigned long lastDataLoopTime;
    void updateData() {
        dataCharacteristicValueBitmask = 0;
        dataCharacteristicValueOffset = IMU_DATA_BASE_OFFSET;
        for (uint8_t i = 0; i < NUMBER_OF_DATA_TYPES; i++) {
            if (delays[i] != 0 && lastDataLoopTime % delays[i] == 0) {
                switch(i) {
                    case ACCELERATION:
                        addData(i, true, Adafruit_BNO055::VECTOR_ACCELEROMETER);
                        break;
                    case GRAVITY:
                        addData(i, true, Adafruit_BNO055::VECTOR_GRAVITY);
                        break;
                    case LINEAR_ACCELERATION:
                        addData(i, true, Adafruit_BNO055::VECTOR_LINEARACCEL);
                        break;
                    case ROTATION_RATE:
                        addData(i, true, Adafruit_BNO055::VECTOR_GYROSCOPE);
                        break;
                    case MAGNETOMETER:
                        addData(i, true, Adafruit_BNO055::VECTOR_MAGNETOMETER);
                        break;
                    case QUATERNION:
                        addData(i);
                        break;
                    default:
                        break;
                }
            }
        }
        if (dataCharacteristicValueBitmask != 0) {
            sendData();
        }
    }
    void addData(uint8_t bitIndex, bool isVector, Adafruit_BNO055::adafruit_vector_type_t vector_type) {
        uint8_t size = isVector ? 6 : 8;
        if (dataCharacteristicValueOffset + size > MAX_CHARACTERISTIC_VALUE_LENGTH)
        {
            sendData();
        }

        bitSet(dataCharacteristicValueBitmask, bitIndex);

        if (isVector)
        {
            bno.getRawVectorData(vector_type, dataCharacteristicValueBuffer);
        }
        else
        {
            bno.getRawQuatData(dataCharacteristicValueBuffer);
        }

        MEMCPY(&dataCharacteristicValue[dataCharacteristicValueOffset], &dataCharacteristicValueBuffer, size);
        dataCharacteristicValueOffset += size;
    }
    void sendData() {
        MEMCPY(&dataCharacteristicValue[0], &dataCharacteristicValueBitmask, 1);
        MEMCPY(&dataCharacteristicValue[1], &currentTime, sizeof(currentTime));
        pDataCharacteristic->setValue((uint8_t *)dataCharacteristicValue, dataCharacteristicValueOffset);
        pDataCharacteristic->notify();

        memset(&dataCharacteristicValue, 0, sizeof(dataCharacteristicValue));
        dataCharacteristicValueBitmask = 0;
        dataCharacteristicValueOffset = IMU_DATA_BASE_OFFSET;
    }
    void dataLoop() {
        if (currentTime >= lastDataLoopTime + IMU_DATA_DELAY_MS) {
            updateData();
            lastDataLoopTime += IMU_DATA_DELAY_MS;
        }
    }

    void setup() {
        if (!bno.begin())
        {
            Serial.println("No BNO055 detected");
        }
        delay(1000);
        bno.setExtCrystalUse(false);
        bno.enterSuspendMode();

        pCalibrationCharacteristic = ble::createCharacteristic(GENERATE_UUID("2000"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "imu calibration");

        pConfigurationCharacteristic = ble::createCharacteristic(GENERATE_UUID("2001"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "imu configuration");
        pConfigurationCharacteristic->setValue((uint8_t *) delays, sizeof(delays));
        pConfigurationCharacteristic->setCallbacks(new ConfigurationCharacteristicCallbacks());

        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("2002"), BLECharacteristic::PROPERTY_NOTIFY, "imu data");
    }

    void start() {
        if (!isBnoAwake) {
            bno.enterNormalMode();
            isBnoAwake = true;
        }
    }
    void stop() {
        if (isBnoAwake) {
            bno.enterSuspendMode();
            isBnoAwake = false;

            memset(&delays, 0, sizeof(delays));
            pConfigurationCharacteristic->setValue((uint8_t *) delays, sizeof(delays));
        }
    }

    unsigned long currentTime;
    void loop() {
        currentTime = millis();

        if (ble::isServerConnected && isBnoAwake) {
            dataLoop();
            calibrationLoop();
        }
    }
} // namespace imu