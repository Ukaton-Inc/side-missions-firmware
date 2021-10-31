#include "definitions.h"
#include "powerManagement.h"
#include "motion.h"
#include "wifiServer.h"

#include "WiFi.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include <esp_wifi.h>

namespace powerManagement
{
    void print_wakeup_reason()
    {
        esp_sleep_wakeup_cause_t wakeup_reason;
        wakeup_reason = esp_sleep_get_wakeup_cause();
        switch (wakeup_reason)
        {
        case ESP_SLEEP_WAKEUP_EXT0:
            Serial.println("Wakeup caused by external signal using RTC_IO");
            break;
        case ESP_SLEEP_WAKEUP_EXT1:
            Serial.println("Wakeup caused by external signal using RTC_CNTL");
            break;
        case ESP_SLEEP_WAKEUP_TIMER:
            Serial.println("Wakeup caused by timer");
            break;
        case ESP_SLEEP_WAKEUP_TOUCHPAD:
            Serial.println("Wakeup caused by touchpad");
            break;
        case ESP_SLEEP_WAKEUP_ULP:
            Serial.println("Wakeup caused by ULP program");
            break;
        default:
            Serial.println("Wakeup was not caused by deep sleep");
            break;
        }
    }

    void setup()
    {
        print_wakeup_reason();
    }

    unsigned long currentTime;
    void loop()
    {
        currentTime = millis();
        if (!wifiServer::isConnectedToClient() && (currentTime - max(wifiServer::lastTimeConnected, motion::lastTimeMoved)) > delayUntilSleep) {
            sleep();
        }
    }

    void sleep()
    {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);

        adc_power_off();
        esp_wifi_stop();

        rtc_gpio_init(motion::interrupt_pin);
        gpio_pullup_dis(motion::interrupt_pin);
        gpio_pulldown_en(motion::interrupt_pin);
        esp_sleep_enable_ext0_wakeup(motion::interrupt_pin, 1);

        motion::bno.setMode(Adafruit_BNO055::OPERATION_MODE_ACCONLY);

        Serial.println("Time to sleep...");
        esp_deep_sleep_start();
    }
} // namespace powerManagement