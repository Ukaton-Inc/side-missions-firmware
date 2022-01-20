#include "wifiServer.h"
#include "udp.h"
#include "sensor/sensorData.h"

namespace udp
{
    AsyncUDP udp;
    IPAddress remoteAddress; 
    uint16_t remotePort;
    bool setStuff = false;
    void onUDPPacket(AsyncUDPPacket packet)
    {
        Serial.print("UDP Packet Type: ");
        Serial.print(packet.isBroadcast() ? "Broadcast" : (packet.isMulticast() ? "Multicast" : "Unicast"));
        Serial.print(", From: ");
        Serial.print(packet.remoteIP());
        Serial.print(":");
        Serial.print(packet.remotePort());
        Serial.print(", To: ");
        Serial.print(packet.localIP());
        Serial.print(":");
        Serial.print(packet.localPort());
        Serial.print(", Length: ");
        Serial.print(packet.length());
        Serial.print(", Data: ");
        auto packetData = packet.data();
        for (uint8_t index = 0; index < packet.length(); index++) {
            Serial.print(packetData[index]);
            Serial.print(", ");
        }
        Serial.println();

        if (!setStuff) {
            setStuff = true;
            remoteAddress = packet.remoteIP();
            remotePort = packet.remotePort();
        }
        //reply to the client
        //packet.printf("Got %u bytes of data", packet.length());
        uint8_t message[5]  = {1, 2, 3, 4, 5};
        packet.write(message, sizeof(message));
        //udp.writeTo(message, sizeof(message), packet.remoteIP(), 9999);
    }
    void listen(uint16_t port) {
        if (udp.listen(port))
        {
            Serial.print("UDP Listening on IP: ");
            Serial.print(WiFi.localIP());
            Serial.print(": ");
            Serial.println(port);
            udp.onPacket([](AsyncUDPPacket packet)
                         { onUDPPacket(packet); });
        }
    }

    unsigned long currentTime = 0;

    unsigned long lastSensorDataUpdateTime;
    void sensorDataLoop()
    {
        /*
        if (currentTime - lastSensorDataUpdateTime > 1000) {
            Serial.println("send");
            lastSensorDataUpdateTime = currentTime;
            uint8_t message[5]  = {9,9,9,9,9};
            udp.writeTo(message, sizeof(message), remoteAddress, remotePort);
        }
        */

        if (lastSensorDataUpdateTime != sensorData::lastDataUpdateTime && (sensorData::motionDataSize + sensorData::pressureDataSize > 0))
        {
            lastSensorDataUpdateTime = sensorData::lastDataUpdateTime;

            // send data
        }
    }
    void loop()
    {
        currentTime = millis();
        sensorDataLoop();
    }
} // namespace udp
