#include "sensors.h"
#include "ble.h"
#include "eepromUtils.h"

namespace sensors {
    Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);
    bool isBnoAwake = false;

    const uint16_t calibration_delay_ms = 1000;
    const uint16_t data_delay_ms = 20;
    bool wroteFullCalibration = false;

    uint8_t callibration[NUMBER_OF_CALIBRATION_TYPES];
    void updateCalibration() {
        bno.getCalibration(&callibration[0], &callibration[1], &callibration[2], &callibration[3]);
        if (bno.isFullyCalibrated() && !wroteFullCalibration) {
            bool saveSuccessful = saveToEEPROM();
            if (saveSuccessful) {
                wroteFullCalibration = true;
            }
        }
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

    // {sensorId, minScaled, maxScaled, minRaw, maxRaw, samplePeriodBitmask, sampleLength} x4
    uint8_t information[(1 + 2 + 2 + 2 + 2 + 2 + 1) * NUMBER_OF_DATA_TYPES] = {0, 255, 248, 0, 8, 128, 1, 127, 255, 0, 31, 7, 1, 248, 48, 7, 208, 128, 1, 127, 255, 0, 31, 7, 2, 255, 255, 0, 1, 192, 1, 63, 255, 0, 31, 10, 3, 255, 255, 0, 1, 192, 1, 63, 255, 0, 31, 8};
    BLECharacteristic *pInformationCharacteristic;

    uint16_t delays[NUMBER_OF_DATA_TYPES] = {0};
    uint8_t configuration[3 * NUMBER_OF_DATA_TYPES] = {ACCELEROMETER,0,0, GYROSCOPE,0,0, ROTATION,0,0, GAME_ROTATION,0,0};
    BLECharacteristic *pConfigurationCharacteristic;
    class ConfigurationCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            uint8_t length = pCharacteristic->getDataLength();
            if (length == sizeof(configuration)) {
                uint8_t *data = (uint8_t *) pCharacteristic->getValue().data();
                uint8_t length = pCharacteristic->getDataLength();
                for (uint8_t i = 0; i < length; i += 3) {
                    uint8_t sensorId = data[i];
                    if (sensorId < NUMBER_OF_DATA_TYPES) {
                        uint16_t delay = 0;
                        delay += data[i+1] << 8;
                        delay += data[i+2];
                        delay -= delay % data_delay_ms;
                        delays[sensorId] = delay;
                        configuration[i+1] = highByte(delay);
                        configuration[i+2] = lowByte(delay);
                    }
                }
            }
            pCharacteristic->setValue(configuration, sizeof(configuration));
            pCharacteristic->notify();
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
        dataCharacteristicValueOffset = 0;
        for (uint8_t dataType = 0; dataType < NUMBER_OF_DATA_TYPES; dataType++) {
            if (delays[dataType] != 0 && lastDataLoopTime % delays[dataType] == 0) {
                switch(dataType) {
                    case ACCELEROMETER:
                        addData(dataType, true, Adafruit_BNO055::VECTOR_ACCELEROMETER);
                        break;
                    case GYROSCOPE:
                        addData(dataType, true, Adafruit_BNO055::VECTOR_GYROSCOPE);
                        break;
                    case ROTATION:
                        addData(dataType);
                        break;
                    case GAME_ROTATION:
                        addData(dataType);
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
    void addData(uint8_t dataType, bool isVector, Adafruit_BNO055::adafruit_vector_type_t vector_type) {
        uint8_t size = sizeof(uint8_t) + sizeof(uint16_t); // sensorId + timestamp
        uint8_t dataSize = isVector? 6:8;
        size += dataSize;
        if (isVector) {
            size += sizeof(uint8_t); // accuracy
        }
        else {
            if (dataType == ROTATION) {
                size += sizeof(uint16_t); // accuracy
            }
        }

        if (dataCharacteristicValueOffset + size > ble::max_characteristic_value_length)
        {
            sendData();
        }

        bitSet(dataCharacteristicValueBitmask, dataType);

        if (isVector)
        {
            bno.getRawVectorData(vector_type, dataCharacteristicValueBuffer);
        }
        else
        {
            bno.getRawQuatData(dataCharacteristicValueBuffer);
        }

        dataCharacteristicValue[dataCharacteristicValueOffset++] = dataType;
        dataCharacteristicValue[dataCharacteristicValueOffset++] = highByte((uint16_t) currentTime);
        dataCharacteristicValue[dataCharacteristicValueOffset++] = lowByte((uint16_t) currentTime);
        if (isVector) {
            int16_t x = dataCharacteristicValueBuffer[0];
            int16_t y = dataCharacteristicValueBuffer[1];
            int16_t z = -dataCharacteristicValueBuffer[2];
            dataCharacteristicValue[dataCharacteristicValueOffset+0] = highByte(x);
            dataCharacteristicValue[dataCharacteristicValueOffset+1] = lowByte(x);

            dataCharacteristicValue[dataCharacteristicValueOffset+2] = highByte(y);
            dataCharacteristicValue[dataCharacteristicValueOffset+3] = lowByte(y);

            dataCharacteristicValue[dataCharacteristicValueOffset+4] = highByte(z);
            dataCharacteristicValue[dataCharacteristicValueOffset+5] = lowByte(z);
        }
        else {
            int16_t w = -dataCharacteristicValueBuffer[0];
            int16_t x = -dataCharacteristicValueBuffer[1];
            int16_t y = dataCharacteristicValueBuffer[2];
            int16_t z = dataCharacteristicValueBuffer[3];
            dataCharacteristicValue[dataCharacteristicValueOffset+0] = highByte(z);
            dataCharacteristicValue[dataCharacteristicValueOffset+1] = lowByte(z);

            dataCharacteristicValue[dataCharacteristicValueOffset+2] = highByte(w);
            dataCharacteristicValue[dataCharacteristicValueOffset+3] = lowByte(w);

            dataCharacteristicValue[dataCharacteristicValueOffset+4] = highByte(x);
            dataCharacteristicValue[dataCharacteristicValueOffset+5] = lowByte(x);

            dataCharacteristicValue[dataCharacteristicValueOffset+6] = highByte(y);
            dataCharacteristicValue[dataCharacteristicValueOffset+7] = lowByte(y);
        }
        dataCharacteristicValueOffset += dataSize;
        if (isVector) {
            dataCharacteristicValue[dataCharacteristicValueOffset++] = 3;
        }
        else {
            if (dataType == ROTATION) {
                dataCharacteristicValue[dataCharacteristicValueOffset++] = highByte(20000);
                dataCharacteristicValue[dataCharacteristicValueOffset++] = lowByte(20000);
            }
        }
    }
    void sendData() {
        pDataCharacteristic->setValue((uint8_t *)dataCharacteristicValue, dataCharacteristicValueOffset);
        pDataCharacteristic->notify();

        memset(&dataCharacteristicValue, 0, sizeof(dataCharacteristicValue));
        dataCharacteristicValueBitmask = 0;
        dataCharacteristicValueOffset = 0;
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

        pInformationCharacteristic = ble::createCharacteristic("855CB3E7-98FF-42A6-80FC-40B32A2221C1", NIMBLE_PROPERTY::READ, "Sensor information");
        pInformationCharacteristic->setValue((uint8_t *) information, sizeof(information));

        pConfigurationCharacteristic = ble::createCharacteristic("5AF38AF6-000E-404B-9B46-07F77580890B", NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE | NIMBLE_PROPERTY::NOTIFY, "Sensor configuration");
        pConfigurationCharacteristic->setValue((uint8_t *) configuration, sizeof(configuration));
        pConfigurationCharacteristic->setCallbacks(new ConfigurationCharacteristicCallbacks());

        pDataCharacteristic = ble::createCharacteristic("56A72AB8-4988-4CC8-A752-FBD1D54A953D", NIMBLE_PROPERTY::NOTIFY, "Sensor data");
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

            for (uint8_t delayType = 0; delayType < NUMBER_OF_DATA_TYPES; delayType++) {
                delays[delayType] = 0;
                configuration[delayType*3 +1] = 0;
                configuration[delayType*3 +2] = 0;
            }
            pConfigurationCharacteristic->setValue(configuration, sizeof(configuration));
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
} // namespace sensors