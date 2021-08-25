#include "motion.h"
#include "eepromUtils.h"
#include "websocketServer.h"

namespace motion
{
    Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);
    bool isBnoAwake = false;

    const uint16_t calibration_delay_ms = 1000;
    const uint16_t data_delay_ms = 40;
    bool wroteFullCalibration = false;

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

    unsigned long lastDataLoopTime;
    void updateData()
    {
        uint8_t data[3] = {1, 2, 3};
        websocketServer::ws.binaryAll(data, sizeof(data));
    }
    void dataLoop()
    {
        if (currentTime >= lastDataLoopTime + data_delay_ms)
        {
            updateData();
            lastDataLoopTime = currentTime - (currentTime % data_delay_ms);
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
        bno.setExtCrystalUse(false);
        bno.enterSuspendMode();
    }

    void start()
    {
        loadFromEEPROM();
        if (!isBnoAwake)
        {
            bno.enterNormalMode();
            isBnoAwake = true;
        }
    }
    void stop()
    {
        if (isBnoAwake)
        {
            bno.enterSuspendMode();
            isBnoAwake = false;
        }
    }

    unsigned long currentTime;
    void loop()
    {
        currentTime = millis();

        if (isBnoAwake)
        {
            dataLoop();
            calibrationLoop();
        }
    }
} // namespace motion