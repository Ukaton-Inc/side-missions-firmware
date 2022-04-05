#include "steps.h"
#include "sensor/pressureSensor.h"
#include "information/type.h"
#include <Preferences.h>

namespace steps
{
    Preferences preferences;

    bool _isTracking = false;
    bool isTracking()
    {
        return _isTracking;
    }
    void loadIsTracking()
    {
        preferences.begin("steps");
        if (preferences.isKey("isTracking"))
        {
            _isTracking = preferences.getBool("isTracking", true);
        }
        preferences.end();
    }
    void saveIsTracking()
    {
        preferences.begin("steps");
        preferences.putBool("isTracking", _isTracking);
        preferences.end();
    }
    void startTracking()
    {
        if (!_isTracking)
        {
            Serial.println("started tracking");
            _isTracking = true;
            saveIsTracking();
        }
    }
    void stopTracking()
    {
        if (_isTracking)
        {
            Serial.println("stopped tracking");
            _isTracking = false;
            saveIsTracking();
        }
    }

    uint32_t steps = 0;
    void loadSteps()
    {
        preferences.begin("steps");
        if (preferences.isKey("steps"))
        {
            steps = preferences.getULong("steps", steps);
        }
        preferences.end();
    }
    void saveSteps()
    {
        preferences.begin("steps");
        preferences.putULong("steps", steps);
        preferences.end();
    }
    uint32_t getSteps()
    {
        return steps;
    }
    void setSteps(uint32_t _steps)
    {
        if (steps != _steps)
        {
            steps = _steps;
            saveSteps();
        }
    }
    constexpr uint16_t save_step_interval = 10; // save to preferences every 100 steps
    void onStep()
    {
        steps++;
        if (steps % save_step_interval == 0)
        {
            saveSteps();
        }
        lastTimeStepsWereUpdated = millis();
    }
    void resetSteps()
    {
        setSteps(0);
    }

    float massThreshold = 0.04;
    float getMassThreshold()
    {
        return massThreshold;
    }
    void loadMassThreshold()
    {
        preferences.begin("steps");
        if (preferences.isKey("massThreshold"))
        {
            massThreshold = preferences.getFloat("massThreshold", massThreshold);
        }
        preferences.end();
    }
    void saveMassThreshold()
    {
        preferences.begin("steps");
        preferences.putFloat("massThreshold", massThreshold);
        preferences.end();
    }
    void setMassThreshold(float _massThreshold)
    {
        if (_massThreshold != massThreshold)
        {
            massThreshold = _massThreshold;
            saveMassThreshold();
        }
    }

    unsigned long lastStepCheckTime = 0;
    unsigned long lastTimeStepsWereUpdated = 0;
    constexpr uint16_t check_step_delay_ms = 100;
    constexpr float massScalar = (float)pow(2, 12) * (float)pressureSensor::number_of_pressure_sensors;
    bool isFootOnGround = false;
    void checkStep()
    {
        pressureSensor::update();
        auto rawMass = *(pressureSensor::getMass());
        float mass = (float)rawMass / massScalar;
        //Serial.printf("mass: %f\n", mass);
        bool _isFootOnGround = mass > massThreshold;
        if (_isFootOnGround != isFootOnGround)
        {
            isFootOnGround = _isFootOnGround;
            //Serial.printf("foot is %son the ground\n", isFootOnGround ? "" : "not ");
            if (!isFootOnGround)
            {
                onStep();
            }
        }
    }

    void setup()
    {
        loadIsTracking();
        loadSteps();
        loadMassThreshold();
    }

    unsigned long currentTime = 0;
    void loop()
    {
        currentTime = millis();

        if (_isTracking && type::isInsole() && currentTime >= lastStepCheckTime + check_step_delay_ms)
        {
            checkStep();
            lastStepCheckTime = currentTime - (currentTime % check_step_delay_ms);
        }
    }
} // namespace steps
