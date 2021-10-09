#include "motion.h"
#include "eepromUtils.h"
#include "websocketServer.h"

namespace motion
{
    Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);
    bool isBnoAwake = false;

    bool wroteFullCalibration = false;

    unsigned long lastTimeMoved = 0;
    bool didInterrupt = false;
    void interruptCallback()
    {
        didInterrupt = true;
        lastTimeMoved = millis();
    }

    const uint16_t calibration_delay_ms = 1000;
    uint8_t callibration[NUMBER_OF_CALIBRATION_TYPES];
    void updateCalibration()
    {
        bno.getCalibration(&callibration[0], &callibration[1], &callibration[2], &callibration[3]);
        if (bno.isFullyCalibrated() && !wroteFullCalibration)
        {
            bool saveSuccessful = saveToEEPROM();
            if (saveSuccessful)
            {
                wroteFullCalibration = true;
            }
        }
    }

    unsigned long lastCalibrationLoopTime = 0;
    void calibrationLoop()
    {
        if (currentTime >= lastCalibrationLoopTime + calibration_delay_ms)
        {
            lastCalibrationLoopTime = currentTime - (currentTime % calibration_delay_ms);
            updateCalibration();
        }
    }

    uint16_t eepromAddress;
    adafruit_bno055_offsets_t calibrationData;

    void loadFromEEPROM()
    {
        EEPROM.get(eepromAddress, calibrationData);
        bno.setSensorOffsets(calibrationData);
    }
    bool saveToEEPROM()
    {
        bool readSuccessful = bno.getSensorOffsets(calibrationData);
        if (readSuccessful)
        {
            EEPROM.put(eepromAddress, calibrationData);
            return EEPROM.commit();
        }
        else
        {
            return readSuccessful;
        }
    }

    const uint16_t data_delay_ms = 20;
    uint16_t configuration[NUMBER_OF_DATA_TYPES] = {0};
    void setConfiguration(uint16_t *newConfiguration) {
        for (uint8_t dataType = 0; dataType < NUMBER_OF_DATA_TYPES; dataType++) {
            configuration[dataType] = newConfiguration[dataType];
            configuration[dataType] -= configuration[dataType] % data_delay_ms;
        }
    }
    unsigned long lastDataLoopTime;
    void dataLoop()
    {
        if (currentTime >= lastDataLoopTime + data_delay_ms)
        {
            sendData();
            lastDataLoopTime = currentTime - (currentTime % data_delay_ms);
        }
    }

    void sendData()
    {
        uint8_t dataSize = 0;
        uint8_t dataBitmask = 0;
        for (uint8_t dataType = 0; dataType < NUMBER_OF_DATA_TYPES; dataType++)
        {
            if (configuration[dataType] != 0 && lastDataLoopTime % configuration[dataType] == 0)
            {
                bitSet(dataBitmask, dataType);
                dataSize += (dataType == QUATERNION)? 8:6;
            }
        }

        if (dataSize > 0)
        {
            uint8_t dataOffset = 1+1+4;
            uint8_t data[dataOffset + dataSize];
            data[0] = websocketServer::IMU_DATA;
            data[1] = dataBitmask;
            MEMCPY(&data[2], &currentTime, sizeof(currentTime));

            int16_t _data[4] = {0};
            for (uint8_t dataType = 0; dataType < NUMBER_OF_DATA_TYPES; dataType++)
            {
                if (bitRead(dataBitmask, dataType))
                {
                    uint8_t _dataSize = 0;

                    switch (dataType)
                    {
                    case ACCELERATION:
                        bno.getRawVectorData(Adafruit_BNO055::VECTOR_ACCELEROMETER, _data);
                        _dataSize = 6;
                        break;
                    case GRAVITY:
                        bno.getRawVectorData(Adafruit_BNO055::VECTOR_GRAVITY, _data);
                        _dataSize = 6;
                        break;
                    case LINEAR_ACCELERATION:
                        bno.getRawVectorData(Adafruit_BNO055::VECTOR_LINEARACCEL, _data);
                        _dataSize = 6;
                        break;
                    case ROTATION_RATE:
                        bno.getRawVectorData(Adafruit_BNO055::VECTOR_GYROSCOPE, _data);
                        _dataSize = 6;
                        break;
                    case MAGNETOMETER:
                        bno.getRawVectorData(Adafruit_BNO055::VECTOR_MAGNETOMETER, _data);
                        _dataSize = 6;
                        break;
                    case QUATERNION:
                        bno.getRawQuatData(_data);
                        _dataSize = 8;
                        break;
                    default:
                        break;
                    }

                    if (_dataSize > 0) {
                        MEMCPY(&data[dataOffset], &_data, _dataSize);
                        dataOffset += _dataSize;
                    }
                }
            }
            websocketServer::ws.binaryAll(data, sizeof(data));
        }
    }

    void setup()
    {
        if (!bno.begin())
        {
            Serial.println("No BNO055 detected");
        }
        delay(1000);
        eepromAddress = eepromUtils::reserveSpace(sizeof(adafruit_bno055_offsets_t));
        if (eepromUtils::firstInitialized)
        {
            saveToEEPROM();
        }
        else
        {
            loadFromEEPROM();
        }

        pinMode(interrupt_pin, INPUT);
        attachInterrupt(digitalPinToInterrupt(interrupt_pin), interruptCallback, RISING);
        bno.enableAnyMotion(128, 5);
        bno.enableInterruptsOnXYZ(ENABLE, ENABLE, ENABLE);
        bno.setExtCrystalUse(false);
        bno.enterNormalMode();
        isBnoAwake = true;
    }

    void start()
    {
        loadFromEEPROM();
        if (!isBnoAwake)
        {
            //bno.enterNormalMode();
            //isBnoAwake = true;
        }
    }
    void stop()
    {
        if (isBnoAwake)
        {
            //bno.enterSuspendMode();
            //isBnoAwake = false;

            memset(&configuration, 0, sizeof(configuration));
        }
    }

    unsigned long currentTime;
    void loop()
    {
        currentTime = millis();

        if (didInterrupt)
        {
            didInterrupt = false;
            bno.resetInterrupts();
        }

        if (isBnoAwake && websocketServer::ws.count() > 0)
        {
            dataLoop();
            calibrationLoop();
        }
    }
} // namespace motion
