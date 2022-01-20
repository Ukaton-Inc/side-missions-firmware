#include "definitions.h"
#include "motionSensor.h"
#include "eepromUtils.h"
#include <lwipopts.h>

namespace motionSensor
{
    Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);

    bool isValidDataType(DataType dataType)
    {
        return dataType < DataType::COUNT;
    }

    adafruit_bno055_offsets_t sensorOffsets;
    uint8_t calibration[(uint8_t)CalibrationType::COUNT];
    
    uint16_t sensorOffsetsEepromAddress;
    void loadFromEEPROM()
    {
        EEPROM.get(sensorOffsetsEepromAddress, sensorOffsets);
        bno.setSensorOffsets(sensorOffsets);
    }
    bool saveToEEPROM()
    {
        bool readSuccessful = bno.getSensorOffsets(sensorOffsets);
        if (readSuccessful)
        {
            EEPROM.put(sensorOffsetsEepromAddress, sensorOffsets);
            return EEPROM.commit();
        }
        else
        {
            return readSuccessful;
        }
    }

    bool wroteFullCalibration = false;
    void updateCalibration()
    {
        bno.getCalibration(&calibration[0], &calibration[1], &calibration[2], &calibration[3]);
        if (bno.isFullyCalibrated() && !wroteFullCalibration)
        {
            bool saveSuccessful = saveToEEPROM();
            if (saveSuccessful)
            {
                wroteFullCalibration = true;
            }
        }
    }
    unsigned long lastCalibrationUpdateTime = 0;
    const uint16_t calibration_delay_ms = 1000;

    unsigned long lastTimeMoved = 0;
    bool didInterrupt = false;
    void interruptCallback()
    {
#if DEBUG
        Serial.println("detected movement");
#endif
        didInterrupt = true;
        lastTimeMoved = millis();
    }

    void setup()
    {
#if DEBUG
        Serial.println("motion setup...");
#endif
        if (!bno.begin())
        {
            Serial.println("No BNO055 detected");
        }
        delay(1000);

        sensorOffsetsEepromAddress = eepromUtils::reserveSpace(sizeof(adafruit_bno055_offsets_t));
        if (eepromUtils::firstInitialized)
        {
            saveToEEPROM();
        }
        else
        {
            loadFromEEPROM();
        }

        bno.setExtCrystalUse(false);
#if ENABLE_MOVE_TO_WAKE
        pinMode(interrupt_pin, INPUT);
        attachInterrupt(digitalPinToInterrupt(interrupt_pin), interruptCallback, RISING);
        bno.enableAnyMotion(100, 5);
        bno.enableInterruptsOnXYZ(ENABLE, ENABLE, ENABLE);
        bno.enterLowPowerMode();
#else
        bno.enterNormalMode();
#endif

#if DEBUG
        Serial.println("done setting up motion sensor");
#endif
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

        if (currentTime >= lastCalibrationUpdateTime + calibration_delay_ms)
        {
            updateCalibration();
            lastCalibrationUpdateTime = currentTime - (currentTime % calibration_delay_ms);
        }
    }
} // namespace motionSensor