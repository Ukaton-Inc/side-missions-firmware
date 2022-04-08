#include "definitions.h"
#include "weightData.h"
#include "weightDetection.h"
#include "information/type.h"

#include "wifi/webSocket.h"
#include "ble/ble.h"

namespace weightData
{

    const uint16_t min_delay_ms = 20;
    uint16_t delay = 0;
    void setDelay(uint16_t _delay)
    {
        delay = _delay - (_delay % min_delay_ms);
#if DEBUG
        Serial.print("updating weight delay: ");
        Serial.println(delay);
#endif
    }
    uint16_t getDelay()
    {
        return delay;
    }

    float weight;
    float getWeight()
    {
        return weight;
    }

    unsigned long currentTime = 0;
    unsigned long lastDataUpdateTime = 0;
    void loop()
    {
        currentTime = millis();
        if (delay > 0 && type::isInsole() && (currentTime >= (lastDataUpdateTime + delay)) && (ble::isServerConnected || webSocket::isConnectedToClient()))
        {
            weight = weightDetection::guessWeight();
            lastDataUpdateTime = currentTime - (currentTime % delay);
        }
    }
} // namespace weightData