#include "battery.h"
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

namespace battery
{
    unsigned long currentTime = 0;

    SFE_MAX1704X lipo;

    void setup()
    {
        lipo.enableDebugging();
        Serial.println("gonna begin battery gauge");
        if (!lipo.begin())
        {
            Serial.println("MAX17043 not detected. Please check wiring");
        }
        lipo.quickStart();
        //lipo.setThreshold(20);

        
    }

    float voltage = 0;
    float soc = 0;

    float getLevel()
    {
        return soc;
    }

    constexpr uint32_t check_battery_level_delay_ms = 1000 * 10; // check every 10 seconds
    unsigned long lastCheckBatteryLevelTime = 0;
    unsigned long lastUpdateBatteryLevelTime = 0;
    void checkBatteryLevel()
    {
        auto _voltage = lipo.getVoltage();
        auto _soc = lipo.getSOC();

        //Serial.printf("voltage: %fv\n", _voltage);
        //Serial.printf("soc: %f%%\n", _soc);

        if (_soc != soc)
        {
            soc = _soc;
            voltage = _voltage;
            lastUpdateBatteryLevelTime = currentTime;
        }
    }

    void loop()
    {
        currentTime = millis();
        if (currentTime >= lastCheckBatteryLevelTime + check_battery_level_delay_ms)
        {
            checkBatteryLevel();
            lastCheckBatteryLevelTime = currentTime - (currentTime % check_battery_level_delay_ms);
        }
    }
} // namespace battery