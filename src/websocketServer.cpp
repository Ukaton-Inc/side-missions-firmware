#include "websocketServer.h"
#include "name.h"
#include "motion.h"
#include "pressure.h"
#include "battery.h"

namespace websocketServer
{
    const char *ssid = "SSID";
    const char *password = "PASSWORD";
    AsyncWebServer server(80);
    AsyncWebSocket ws("/ws");

    void handleWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len)
        {
            uint8_t messageType = data[0];
            switch (messageType)
            {
            case GET_NAME_REQUEST:
            {
                uint8_t responseData[name::name.length() + 1 + 1];
                responseData[0] = GET_NAME_RESPONSE;
                MEMCPY(&responseData[1], name::name.c_str(), name::name.length() + 1);
                client->binary(responseData, sizeof(responseData));
            }
            break;
            case SET_NAME_REQUEST:
            {
                uint8_t name[len - 1 + 1];
                MEMCPY(&name, &data[1], len - 1);
                name[len - 1] = 0;
                name::setName((char *)name);

                uint8_t responseData[name::name.length() + 1 + 1];
                responseData[0] = SET_NAME_RESPONSE;
                MEMCPY(&responseData[1], name::name.c_str(), name::name.length() + 1);
                client->binary(responseData, sizeof(responseData));
            }
            break;
            case GET_IMU_CALLIBRATION_DATA_REQUEST:
            {
                uint8_t responseData[1 + sizeof(motion::callibration)];
                responseData[0] = GET_IMU_CALLIBRATION_DATA_RESPONSE;
                MEMCPY(&responseData[1], motion::callibration, sizeof(motion::callibration));
                client->binary(responseData, sizeof(responseData));
            }
            break;
            case GET_IMU_CONFIGURATION_REQUEST:
            {
                uint8_t responseData[1 + sizeof(motion::configuration)];
                responseData[0] = GET_IMU_CONFIGURATION_RESPONSE;
                MEMCPY(&responseData[1], motion::configuration, sizeof(motion::configuration));
                client->binary(responseData, sizeof(responseData));
            }
            break;
            case SET_IMU_CONFIGURATION_REQUEST:
                if (len - 1 == sizeof(motion::configuration))
                {
                    uint16_t configuration[motion::NUMBER_OF_DATA_TYPES] = {0};
                    MEMCPY(&configuration[0], &data[1], sizeof(configuration));
                    motion::setConfiguration((uint16_t *)configuration);

                    uint8_t responseData[1 + sizeof(motion::configuration)];
                    responseData[0] = SET_IMU_CONFIGURATION_RESPONSE;
                    MEMCPY(&responseData[1], motion::configuration, sizeof(motion::configuration));
                    client->binary(responseData, sizeof(responseData));
                }
                break;
            case GET_BATTERY_LEVEL_REQUEST:
            {
                uint8_t batteryLevel = battery::batteryLevel;
                uint8_t responseData[1 + sizeof(batteryLevel)];
                responseData[0] = GET_BATTERY_LEVEL_RESPONSE;
                responseData[1] = batteryLevel;
                client->binary(responseData, sizeof(responseData));
            }
            break;
            case GET_PRESSURE_CONFIGURATION_REQUEST:
            {
                uint8_t responseData[1 + sizeof(pressure::configuration)];
                responseData[0] = GET_PRESSURE_CONFIGURATION_RESPONSE;
                MEMCPY(&responseData[1], pressure::configuration, sizeof(pressure::configuration));
                client->binary(responseData, sizeof(responseData));
            }
            break;
            case SET_PRESSURE_CONFIGURATION_REQUEST:
                if (len - 1 == sizeof(pressure::configuration))
                {
                    uint16_t configuration[pressure::NUMBER_OF_DATA_TYPES] = {0};
                    MEMCPY(&configuration[0], &data[1], sizeof(configuration));
                    pressure::setConfiguration((uint16_t *)configuration);

                    uint8_t responseData[1 + sizeof(pressure::configuration)];
                    responseData[0] = SET_PRESSURE_CONFIGURATION_RESPONSE;
                    MEMCPY(&responseData[1], pressure::configuration, sizeof(pressure::configuration));
                    client->binary(responseData, sizeof(responseData));
                }
                break;
            default:
                Serial.print("Unknown message type: ");
                Serial.println(messageType);
                break;
            }
        }
    }

    unsigned long lastTimeConnected = 0;
    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
    {
        switch (type)
        {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            Serial.printf("There are currently %u clients\n", ws.count());
            if (ws.count() > 0)
            {
                motion::start();
            }
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            Serial.printf("There are currently %u clients\n", ws.count());
            if (ws.count() == 0)
            {
                motion::stop();
                pressure::stop();
            }
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(client, arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        }
    }

    void setup()
    {
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
            Serial.println("Connecting to WiFi..");
        }
        Serial.println(WiFi.localIP());

        ws.onEvent(onEvent);
        server.addHandler(&ws);
        server.begin();
    }

    void loop()
    {
        if (ws.count() > 0) {
            lastTimeConnected = millis();
        }
        ws.cleanupClients();
    }
} // namespace websocketServer