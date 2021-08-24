#include "motion.h"
#include "hid.h"
#include "eepromUtils.h"

namespace motion
{
    Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);
    bool isBnoAwake = false;

    const uint16_t calibration_delay_ms = 1000;
    const uint16_t data_delay_ms = 20;
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
        imu::Quaternion quaternion = bno.getQuat();
        imu::Vector<3> euler = quaternion.toEuler();

        // yaw increases as it turns right
        double yaw = euler.x();
        yaw += PI;
        if (yaw > PI) {
            yaw -= TWO_PI;
        }

        // roll increases as it tilts left
        double roll = euler.y();

        // pitch increases as it tilts forward
        double pitch = euler.z();

        Serial.print("roll: ");
        Serial.print(roll);
        Serial.print(", pitch: ");
        Serial.print(pitch);
        Serial.print(", yaw: ");
        Serial.println(yaw);

        int16_t x = 0;
        int16_t y = 0;
        // set x, y based on module orientation
        hid::bleGamepad.setLeftThumb(x, y);
        hid::bleGamepad.sendReport();
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

        if (hid::bleGamepad.isConnected())
        {
            dataLoop();
            calibrationLoop();
        }
    }
} // namespace motion