#include "websocketServer.h"
#include "motion.h"

namespace websocketServer
{
    const char *ssid = "Qattan";
    const char *password = "greenlion222";
    AsyncWebServer server(80);
    AsyncWebSocket ws("/ws");

    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len)
        {
            for (uint8_t index = 0; index < len; index++) {
                Serial.print(index);
                Serial.print(": ");
                Serial.println(data[index]);
            }
        }
    }

    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
    {
        switch (type)
        {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            motion::start();
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            motion::stop();
            break;
        case WS_EVT_DATA:
            handleWebSocketMessage(arg, data, len);
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
        ws.cleanupClients();
    }
} // namespace websocketServer