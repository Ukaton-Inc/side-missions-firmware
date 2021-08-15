#include "motion.h"
#include "ble.h"
#include "eepromUtils.h"

namespace motion {
    Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);
    bool isBnoAwake = false;

    const uint16_t calibration_delay_ms = 1000;
    const uint16_t data_delay_ms = 20;
    const uint8_t data_base_offset = sizeof(uint8_t) + sizeof(unsigned long);
    bool wroteFullCalibration = false;

    BLECharacteristic *pCalibrationCharacteristic;
    uint8_t callibration[NUMBER_OF_CALIBRATION_TYPES];
    void updateCalibration() {
        bno.getCalibration(&callibration[0], &callibration[1], &callibration[2], &callibration[3]);
        if (bno.isFullyCalibrated() && !wroteFullCalibration) {
            bool saveSuccessful = saveToEEPROM();
            if (saveSuccessful) {
                wroteFullCalibration = true;
            }
        }
        pCalibrationCharacteristic->setValue((uint8_t *) callibration, sizeof(callibration));
        pCalibrationCharacteristic->notify();
    }
    unsigned long lastCalibrationLoopTime = 0;
    void calibrationLoop() {
        if (currentTime >= lastCalibrationLoopTime + calibration_delay_ms) {
            lastCalibrationLoopTime = currentTime - (currentTime % calibration_delay_ms);
            updateCalibration();
        }
    }

    uint16_t eepromAddress;
    adafruit_bno055_offsets_t calibrationData;
    sensor_t sensor;

    void loadFromEEPROM() {
        EEPROM.get(eepromAddress, calibrationData);
        bno.setSensorOffsets(calibrationData);
    }
    bool saveToEEPROM() {
        bool readSuccessful = bno.getSensorOffsets(calibrationData);
        if (readSuccessful) {
            EEPROM.put(eepromAddress, calibrationData);
            return EEPROM.commit();
        }
        else {
            return readSuccessful;
        }
    }

    uint16_t delays[NUMBER_OF_DATA_TYPES] = {0};
    BLECharacteristic *pConfigurationCharacteristic;
    class ConfigurationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t length = pCharacteristic->getDataLength();
            if (length == NUMBER_OF_DATA_TYPES * sizeof(uint16_t)) {
                uint16_t *characteristicData = (uint16_t *) pCharacteristic->getValue().data();
                for (int i = 0; i < NUMBER_OF_DATA_TYPES; i++) {
                    delays[i] = characteristicData[i];
                    delays[i] -= delays[i] % data_delay_ms;
                }
            }
            pCharacteristic->setValue((uint8_t *) delays, sizeof(delays));
        }
    };

    BLECharacteristic *pDataCharacteristic;
    uint8_t dataCharacteristicValue[ble::max_characteristic_value_length];
    uint8_t dataCharacteristicValueBitmask;
    uint8_t dataCharacteristicValueOffset;
    int16_t dataCharacteristicValueBuffer[4] = {0};
    unsigned long lastDataLoopTime;
    void updateData() {
        dataCharacteristicValueBitmask = 0;
        dataCharacteristicValueOffset = data_base_offset;
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
        if (dataCharacteristicValueOffset + size > ble::max_characteristic_value_length)
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
        dataCharacteristicValueOffset = data_base_offset;
    }
    void dataLoop() {
        if (currentTime >= lastDataLoopTime + data_delay_ms) {
            updateData();
            lastDataLoopTime = currentTime - (currentTime % data_delay_ms);
        }
    }

    void setup() {
        if (!bno.begin())
        {
            Serial.println("No BNO055 detected");
        }
        delay(1000);
        eepromAddress = eepromUtils::reserveSpace(sizeof(adafruit_bno055_offsets_t));
        if (eepromUtils::firstInitialized) {
            saveToEEPROM();
        }
        else {
            loadFromEEPROM();
        }
        bno.setExtCrystalUse(false);
        bno.enterSuspendMode();

        pCalibrationCharacteristic = ble::createCharacteristic(GENERATE_UUID("2000"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "imu calibration");

        pConfigurationCharacteristic = ble::createCharacteristic(GENERATE_UUID("2001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "imu configuration");
        pConfigurationCharacteristic->setValue((uint8_t *) delays, sizeof(delays));
        pConfigurationCharacteristic->setCallbacks(new ConfigurationCharacteristicCallbacks());

        pDataCharacteristic = ble::createCharacteristic(GENERATE_UUID("2002"), NIMBLE_PROPERTY::NOTIFY, "imu data");
    }

    void start() {
        loadFromEEPROM();
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
} // namespace motion