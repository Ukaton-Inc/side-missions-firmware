#include "moveToWake.h"
#include "motionSensor.h"
#include "definitions.h"

#include "ble/ble.h"

#include "WiFi.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include <esp_wifi.h>
#include <esp_bt.h>

namespace moveToWake
{
    extern bool enabled = ENABLE_MOVE_TO_WAKE;
    unsigned long delayUntilSleep = 1000 * 60;

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
#if DEBUG
        print_wakeup_reason();
#endif
    }

    unsigned long currentTime;
    void loop()
    {
        currentTime = millis();
        if (enabled && !ble::isServerConnected && (currentTime - max(ble::lastTimeConnected, motionSensor::lastTimeMoved)) > delayUntilSleep)
        {
            sleep();
        }
    }

    void sleep()
    {
        WiFi.disconnect(true);
        WiFi.mode(WIFI_OFF);
        btStop();

        adc_power_off();
        esp_wifi_stop();
        esp_bt_controller_disable();

        rtc_gpio_init(motionSensor::interrupt_pin);
        gpio_pullup_dis(motionSensor::interrupt_pin);
        gpio_pulldown_en(motionSensor::interrupt_pin);
        esp_sleep_enable_ext0_wakeup(motionSensor::interrupt_pin, 1);

        Serial.println("Time to sleep...");
        esp_deep_sleep_start();
    }
} // namespace moveToWake