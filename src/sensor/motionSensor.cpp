#include "definitions.h"
#include "motionSensor.h"
#include <Preferences.h>
#include <lwipopts.h>

namespace motionSensor
{
    BNO080 bno;

    bool isValidDataType(DataType dataType)
    {
        return dataType < DataType::COUNT;
    }
    uint8_t calibration[(uint8_t)CalibrationType::COUNT];

    bool wroteFullCalibration = false;
    void updateCalibration()
    {
        calibration[(uint8_t)CalibrationType::ACCELEROMETER] = bno.getAccelAccuracy();
        calibration[(uint8_t)CalibrationType::GYROSCOPE] = bno.getGyroAccuracy();
        calibration[(uint8_t)CalibrationType::MAGNETOMETER] = bno.getMagAccuracy();
        calibration[(uint8_t)CalibrationType::QUATERNION] = bno.getQuatAccuracy();
    }
    unsigned long lastCalibrationUpdateTime = 0;
    const uint16_t calibration_delay_ms = 1000;

    bool didInterrupt = false;
    unsigned long lastTimeMoved = 0;
    void interruptCallback()
    {
        Serial.println("interrupted");
        didInterrupt = true;
    }

    void setup()
    {
        return;
        Wire.begin();
        if (!bno.begin())
        {
            Serial.println("No BNO080 detected");
        }
        Wire.setClock(400000);
        bno.calibrateAll();

        // FILL - external oscillator stuff
        // FILL - interrupts
    }

    unsigned long currentTime;
    void loop()
    {
        currentTime = millis();

        if (didInterrupt)
        {
            didInterrupt = false;
        }

        if (currentTime >= lastCalibrationUpdateTime + calibration_delay_ms)
        {
            updateCalibration();
            lastCalibrationUpdateTime = currentTime - (currentTime % calibration_delay_ms);
        }
    }
} // namespace motionSensor