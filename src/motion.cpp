#include "motion.h"
#include "eepromUtils.h"
#include <lwipopts.h>

namespace motion
{
    Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);

    unsigned long lastTimeMoved = 0;
    bool didInterrupt = false;
    void interruptCallback()
    {
        didInterrupt = true;
        lastTimeMoved = millis();
    }

    adafruit_bno055_offsets_t sensorOffsets;
    uint8_t calibration[(uint8_t) CalibrationType::COUNT];
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
    unsigned long lastCalibrationLoopTime = 0;
    const uint16_t calibration_delay_ms = 1000;

    uint16_t eepromAddress;
    void loadFromEEPROM()
    {
        EEPROM.get(eepromAddress, sensorOffsets);
        bno.setSensorOffsets(sensorOffsets);
    }
    bool saveToEEPROM()
    {
        bool readSuccessful = bno.getSensorOffsets(sensorOffsets);
        if (readSuccessful)
        {
            EEPROM.put(eepromAddress, sensorOffsets);
            return EEPROM.commit();
        }
        else
        {
            return readSuccessful;
        }
    }

    void setup() {
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
        bno.enableAnyMotion(100, 5);
        bno.enableInterruptsOnXYZ(ENABLE, ENABLE, ENABLE);
        bno.setExtCrystalUse(false);
        bno.enterNormalMode(); // it should enter low power mode but we didn't setup an external crystal oscillator so it doesn't work when esp is asleep
    }

    unsigned long currentTime;
    void loop() {
        currentTime = millis();
        
        if (didInterrupt)
        {
            didInterrupt = false;
            bno.resetInterrupts();
        }

        if (currentTime >= lastCalibrationLoopTime + calibration_delay_ms)
        {
            lastCalibrationLoopTime = currentTime - (currentTime % calibration_delay_ms);
            updateCalibration();
        }
    }
} // namespace motion