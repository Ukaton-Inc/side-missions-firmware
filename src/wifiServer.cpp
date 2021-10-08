#include "wifiServer.h"
#include "name.h"
#include "battery.h"
#include "motion.h"

#include <esp_now.h>
#include <esp_wifi.h>

#include "EspNowPeer.h"

namespace wifiServer
{
    const char *ssid = WIFI_SSID;
    const char *password = WIFI_PASSWORD;

    unsigned long currentMillis = 0;
    unsigned long lastTimeConnected = 0;

    bool sentClientNumberOfDevices = false;

    unsigned long previousBatteryLevelMillis = 0;
    const long batteryLevelInterval = 20000;

    unsigned long previousMotionCalibrationMillis = 0;
    const long motionCalibrationInterval = 1000;

    DeviceType deviceType = IS_INSOLE ? (IS_RIGHT_INSOLE ? DeviceType::RIGHT_INSOLE : DeviceType::LEFT_INSOLE) : DeviceType::MOTION_MODULE;

    bool areMacAddressesEqual(const uint8_t *a, const uint8_t *b)
    {
        bool areEqual = true;
        for (uint8_t i = 0; i < MAC_ADDRESS_SIZE && areEqual; i++)
        {
            areEqual = areEqual && (a[i] == b[i]);
        }
        return areEqual;
    }

#if IS_ESP_NOW_RECEIVER

    AsyncWebServer server(80);

    AsyncWebSocket webSocket("/ws");

    bool isConnectedToClient()
    {
        return webSocket.count() > 0;
    }

    uint8_t getNumberOfDevices()
    {
        return EspNowPeer::getNumberOfPeers() + 1;
    }

    std::map<MessageType, std::vector<uint8_t>> clientMessageMap;
    std::map<uint8_t, std::map<MessageType, std::vector<uint8_t>>> deviceClientMessageMaps;
    bool shouldSendToClient = false;

    void onClientConnection()
    {
        EspNowPeer::OnClientConnection();
    }
    void onClientDisconnection()
    {
        EspNowPeer::OnClientDisconnection();
        sentClientNumberOfDevices = false;
    }

    void onClientRequestNumberOfDevices()
    {
        uint8_t numberOfDevices = getNumberOfDevices();
        std::vector<uint8_t> data;
        data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
        data.push_back(numberOfDevices);
        data.push_back(1); // receiver is always available
        for (uint8_t deviceIndex = 1; deviceIndex < numberOfDevices; deviceIndex++)
        {
            auto peer = EspNowPeer::getPeerByDeviceIndex(deviceIndex);
            data.push_back(peer->getAvailability() ? 1 : 0);
        }
        clientMessageMap[MessageType::GET_NUMBER_OF_DEVICES] = data;
        sentClientNumberOfDevices = true;
    }
    uint8_t onClientRequestGetName(uint8_t *data, uint8_t dataOffset)
    {
        uint8_t deviceIndex = data[dataOffset++];
        if (deviceIndex == 0)
        {
            auto myName = name::getName();

            std::vector<uint8_t> data;
            data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
            data.push_back(myName->length());
            data.insert(data.end(), myName->begin(), myName->end());
            deviceClientMessageMaps[deviceIndex][MessageType::GET_NAME] = data;
        }
        else
        {
            try
            {
                auto peer = EspNowPeer::getPeerByDeviceIndex(deviceIndex);
                if (peer->didUpdateNameAtLeastOnce)
                {
                    if (deviceClientMessageMaps[deviceIndex].count(MessageType::SET_NAME) == 0)
                    {
                        std::vector<uint8_t> data;
                        data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
                        auto peerName = peer->getName();
                        data.push_back(peerName->length());
                        data.insert(data.end(), peerName->begin(), peerName->end());
                        deviceClientMessageMaps[deviceIndex][MessageType::GET_NAME] = data;
                    }
                }
                else
                {
                    if (peer->messageMap.count(MessageType::SET_NAME) == 0)
                    {
                        peer->messageMap[MessageType::GET_NAME];
                    }
                }
            }
            catch (const std::out_of_range &error)
            {
                uint8_t data[1];
                data[0] = (uint8_t)ErrorMessageType::DEVICE_NOT_FOUND;
                deviceClientMessageMaps[deviceIndex][MessageType::GET_NAME].assign(data, data + sizeof(data));
            }
        }

        return dataOffset;
    }
    uint8_t onClientRequestSetName(uint8_t *data, uint8_t dataOffset)
    {
        uint8_t deviceIndex = data[dataOffset++];
        uint8_t nameLength = data[dataOffset++];
        const char *newName = (char *)&data[dataOffset];
        dataOffset += nameLength;
        if (deviceIndex == 0)
        {
            name::setName(newName, nameLength);
            const std::string *myName = name::getName();

            std::vector<uint8_t> data;
            data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
            data.push_back(myName->length());
            data.insert(data.end(), myName->begin(), myName->end());
            deviceClientMessageMaps[deviceIndex][MessageType::SET_NAME] = data;
        }
        else
        {
            try
            {
                auto peer = EspNowPeer::getPeerByDeviceIndex(deviceIndex);
                std::vector<uint8_t> data;
                data.push_back(nameLength);
                data.insert(data.end(), newName, newName + nameLength);
                peer->messageMap.erase(MessageType::GET_NAME);
                peer->messageMap[MessageType::SET_NAME] = data;
            }
            catch (const std::out_of_range &error)
            {
                uint8_t data[1];
                data[0] = (uint8_t)ErrorMessageType::DEVICE_NOT_FOUND;
                deviceClientMessageMaps[deviceIndex][MessageType::SET_NAME].assign(data, data + sizeof(data));
            }
        }

        return dataOffset;
    }
    uint8_t onClientRequestGetType(uint8_t *data, uint8_t dataOffset)
    {
        uint8_t deviceIndex = data[dataOffset++];
        if (deviceIndex == 0)
        {
            uint8_t data[2];
            data[0] = (uint8_t)ErrorMessageType::NO_ERROR;
            data[1] = (uint8_t)deviceType;
            deviceClientMessageMaps[deviceIndex][MessageType::GET_TYPE].assign(data, data + sizeof(data));
        }
        else
        {
            try
            {
                auto peer = EspNowPeer::getPeerByDeviceIndex(deviceIndex);
                if (peer->didUpdateNameAtLeastOnce)
                {
                    uint8_t data[2];
                    data[0] = (uint8_t)ErrorMessageType::NO_ERROR;
                    data[1] = (uint8_t)peer->getDeviceType();
                    deviceClientMessageMaps[deviceIndex][MessageType::GET_TYPE].assign(data, data + sizeof(data));
                }
                else
                {
                    peer->messageMap[MessageType::GET_TYPE];
                }
            }
            catch (const std::out_of_range &error)
            {
                uint8_t data[1];
                data[0] = (uint8_t)ErrorMessageType::DEVICE_NOT_FOUND;
                deviceClientMessageMaps[deviceIndex][MessageType::GET_TYPE].assign(data, data + sizeof(data));
            }
        }

        return dataOffset;
    }

    void onWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len)
    {
        auto info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len)
        {
            Serial.print("WebSocket: ");
            for (uint8_t index = 0; index < len; index++)
            {
                Serial.print(data[index]);
                Serial.print(',');
            }
            Serial.println();

            uint8_t dataOffset = 0;
            MessageType messageType;
            while (dataOffset < len)
            {
                messageType = (MessageType)data[dataOffset++];
                Serial.print("Message Type from client: ");
                Serial.println((uint8_t)messageType);

                switch (messageType)
                {
                case MessageType::GET_NUMBER_OF_DEVICES:
                    onClientRequestNumberOfDevices();
                    break;
                case MessageType::GET_NAME:
                    dataOffset = onClientRequestGetName(data, dataOffset);
                    break;
                case MessageType::SET_NAME:
                    dataOffset = onClientRequestSetName(data, dataOffset);
                    break;
                case MessageType::GET_TYPE:
                    dataOffset = onClientRequestGetType(data, dataOffset);
                    break;
                default:
                    Serial.print("uncaught websocket message type: ");
                    Serial.println((uint8_t)messageType);
                    dataOffset = len;
                    break;
                }
            }

            shouldSendToClient = shouldSendToClient || (clientMessageMap.size() > 0) || (deviceClientMessageMaps.size() > 0);
        }
    }

    void onWebSocketEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
    {
        switch (type)
        {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
            Serial.printf("There are currently %u clients\n", webSocket.count());
            if (webSocket.count() == 1)
            {
                onClientConnection();
            }
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", client->id());
            Serial.printf("There are currently %u clients\n", webSocket.count());
            if (webSocket.count() == 0)
            {
                onClientDisconnection();
            }
            break;
        case WS_EVT_DATA:
            Serial.printf("Message of size #%u\n", len);
            onWebSocketMessage(client, arg, data, len);
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        }
    }

    void webSocketLoop()
    {
        if (isConnectedToClient())
        {
            lastTimeConnected = currentMillis;

            if (shouldSendToClient)
            {
                std::vector<uint8_t> clientMessageData;

                if (clientMessageMap.size() > 0)
                {
                    for (auto clientMessageIterator = clientMessageMap.begin(); clientMessageIterator != clientMessageMap.end(); clientMessageIterator++)
                    {
                        clientMessageData.push_back((uint8_t)clientMessageIterator->first);
                        clientMessageData.insert(clientMessageData.end(), clientMessageIterator->second.begin(), clientMessageIterator->second.end());
                    }
                }

                if (deviceClientMessageMaps.size() > 0)
                {
                    for (auto deviceClientMessagesIterator = deviceClientMessageMaps.begin(); deviceClientMessagesIterator != deviceClientMessageMaps.end(); deviceClientMessagesIterator++)
                    {
                        Serial.print("INDEX: ");
                        Serial.println((uint8_t)deviceClientMessagesIterator->first);
                        for (auto deviceClientMessageIterator = deviceClientMessagesIterator->second.begin(); deviceClientMessageIterator != deviceClientMessagesIterator->second.end(); deviceClientMessageIterator++)
                        {
                            Serial.print("MESSAGE TYPE: ");
                            Serial.println((uint8_t)deviceClientMessageIterator->first);
                            clientMessageData.push_back((uint8_t)deviceClientMessageIterator->first);
                            clientMessageData.push_back(deviceClientMessagesIterator->first);
                            clientMessageData.insert(clientMessageData.end(), deviceClientMessageIterator->second.begin(), deviceClientMessageIterator->second.end());
                        }
                    }
                }

                Serial.print("Sending to client...");
                Serial.print(clientMessageMap.size());
                Serial.print(',');
                Serial.print(deviceClientMessageMaps.size());
                Serial.print(',');
                Serial.println(clientMessageData.size());
                for (auto clientMessageIterator = clientMessageData.begin(); clientMessageIterator != clientMessageData.end(); clientMessageIterator++)
                {
                    Serial.print(*clientMessageIterator);
                    Serial.print(',');
                }
                Serial.println();

                webSocket.binaryAll(clientMessageData.data(), clientMessageData.size());
                clientMessageMap.clear();
                deviceClientMessageMaps.clear();

                shouldSendToClient = false;
            }
        }
        webSocket.cleanupClients();
    }

    // ESP NOW
    void onEspNowDataReceived(const uint8_t *macAddress, const uint8_t *incomingData, int len)
    {
        char macAddressString[18];
        Serial.print("Packet received from: ");
        snprintf(macAddressString, sizeof(macAddressString), "%02x:%02x:%02x:%02x:%02x:%02x",
                 macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
        Serial.println(macAddressString);

        auto peer = EspNowPeer::getPeerByMacAddress(macAddress);
        if (peer == nullptr)
        {
            peer = new EspNowPeer(macAddress);
            if (isConnectedToClient())
            {
                deviceClientMessageMaps[peer->getDeviceIndex()][MessageType::DEVICE_ADDED];
                peer->onClientConnection();
            }
            else
            {
                peer->onClientDisconnection();
            }
        }

        if (!peer->isConnected())
        {
            peer->connect();
        }
        else
        {
            Serial.println("already connected to peer");
        }

        peer->onMessage(incomingData, len);
    }
    void onEspNowDataSent(const uint8_t *macAddress, esp_now_send_status_t status)
    {
        auto peer = EspNowPeer::getPeerByMacAddress(macAddress);
        if (peer != nullptr)
        {
            Serial.print("\r\nLast Packet Send Status:\t");
            if (status == ESP_NOW_SEND_SUCCESS)
            {
                Serial.println("Delivery Success");
                peer->updateAvailability(true);
            }
            else
            {
                Serial.print("Delivery Failed: ");
                Serial.println(status);

                peer->updateAvailability(false);

                uint8_t deviceIndex = peer->getDeviceIndex();
                uint8_t errorData[1]{(uint8_t)ErrorMessageType::FAILED_TO_SEND};
                for (auto messageMapIterator = peer->messageMap.begin(); messageMapIterator != peer->messageMap.end(); messageMapIterator++)
                {
                    if (messageMapIterator->first != MessageType::PING)
                    {
                        deviceClientMessageMaps[deviceIndex][messageMapIterator->first].assign(errorData, errorData + sizeof(errorData));
                    }
                }
            }

            peer->messageMap.clear();
        }
    }

    void setup()
    {
        WiFi.mode(WIFI_AP_STA);

        WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(1000);
            Serial.println("Setting as a Wi-Fi Station..");
        }
        Serial.print("Station IP Address: ");
        Serial.println(WiFi.localIP());
        Serial.print("Wi-Fi Channel: ");
        Serial.println(WiFi.channel());
        Serial.print("ESP Board MAC Address:  ");
        Serial.println(WiFi.macAddress());

        if (esp_now_init() != ESP_OK)
        {
            Serial.println("Error initializing ESP-NOW");
            return;
        }

        esp_now_register_recv_cb(onEspNowDataReceived);
        esp_now_register_send_cb(onEspNowDataSent);

        webSocket.onEvent(onWebSocketEvent);
        server.addHandler(&webSocket);

        server.begin();
    }

    void batteryLevelLoop()
    {
        if (currentMillis - previousBatteryLevelMillis >= batteryLevelInterval)
        {
            previousBatteryLevelMillis = currentMillis;
            if (isConnectedToClient())
            {
                uint8_t data[1]{battery::getBatteryLevel()};
                deviceClientMessageMaps[0][MessageType::BATTERY_LEVEL].assign(data, data + sizeof(data));
                shouldSendToClient = true;

                EspNowPeer::batteryLevelsLoop();
            }
        }
    }

    void motionCalibrationLoop()
    {
        if (currentMillis - previousMotionCalibrationMillis >= motionCalibrationInterval)
        {
            previousMotionCalibrationMillis = currentMillis;
            deviceClientMessageMaps[0][MessageType::MOTION_CALIBRATION].assign(motion::calibration, motion::calibration + sizeof(motion::calibration));
            shouldSendToClient = true;

            EspNowPeer::motionCalibrationsLoop();
        }
    }

    void loop()
    {
        currentMillis = millis();

        if (sentClientNumberOfDevices)
        {
            batteryLevelLoop();
            motionCalibrationLoop();
        }
        EspNowPeer::pingLoop();
        EspNowPeer::sendAll();
        webSocketLoop();
    }

#else
    uint8_t receiverMacAddress[] = {0x4C, 0x11, 0xAE, 0x90, 0xE0, 0xC0};
    bool isConnectedToReceiver = false;

    bool _isConnectedToClient = false;
    bool isConnectedToClient()
    {
        return isConnectedToReceiver && _isConnectedToClient;
    }

    std::map<MessageType, std::vector<uint8_t>> receiverMessageMap;
    bool shouldSendToReceiver = false;

    int32_t getWiFiChannel(const char *ssid)
    {
        if (int32_t n = WiFi.scanNetworks())
        {
            for (uint8_t i = 0; i < n; i++)
            {
                if (!strcmp(ssid, WiFi.SSID(i).c_str()))
                {
                    return WiFi.channel(i);
                }
            }
        }
        return 0;
    }

    // ESP NOW
    void onEspNowDataReceived(const uint8_t *macAddress, const uint8_t *incomingData, int len)
    {
        char macAddressString[18];
        Serial.print("Packet received from: ");
        snprintf(macAddressString, sizeof(macAddressString), "%02x:%02x:%02x:%02x:%02x:%02x",
                 macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
        Serial.println(macAddressString);

        if (areMacAddressesEqual(macAddress, receiverMacAddress))
        {
            Serial.print("EspNowReceiver: ");
            for (uint8_t index = 0; index < len; index++)
            {
                Serial.print(incomingData[index]);
                Serial.print(',');
            }
            Serial.println();

            uint8_t incomingDataOffset = 0;
            MessageType messageType;
            while (incomingDataOffset < len)
            {
                messageType = (MessageType)incomingData[incomingDataOffset++];

                Serial.print("Message Type from receiver: ");
                Serial.println((uint8_t)messageType);

                switch (messageType)
                {
                case MessageType::GET_NAME:
                {
                    auto nameString = name::getName();

                    std::vector<uint8_t> data;
                    data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
                    data.push_back(nameString->length());
                    data.insert(data.end(), nameString->begin(), nameString->end());
                    if (receiverMessageMap.count(MessageType::SET_NAME) == 0)
                    {
                        receiverMessageMap[messageType] = data;
                    }
                }
                break;
                case MessageType::SET_NAME:
                {
                    uint8_t nameLength = incomingData[incomingDataOffset++];
                    auto newName = (const char *)&incomingData[incomingDataOffset];
                    incomingDataOffset += nameLength;

                    name::setName(newName, nameLength);
                    auto nameString = name::getName();

                    std::vector<uint8_t> data;
                    data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
                    data.push_back(nameString->length());
                    data.insert(data.end(), nameString->begin(), nameString->end());
                    receiverMessageMap.erase(MessageType::GET_NAME);
                    receiverMessageMap[messageType] = data;
                }
                break;
                case MessageType::GET_TYPE:
                {
                    uint8_t data[2];
                    data[0] = (uint8_t)ErrorMessageType::NO_ERROR;
                    data[1] = (uint8_t)deviceType;
                    receiverMessageMap[messageType].assign(data, data + sizeof(data));
                }
                break;
                case MessageType::PING:
                    break;
                case MessageType::CLIENT_CONNECTED:
                    Serial.println("connected to client");
                    _isConnectedToClient = true;
                    break;
                case MessageType::CLIENT_DISCONNECTED:
                    Serial.println("disconnected from client");
                    _isConnectedToClient = false;
                    break;
                default:
                    Serial.print("uncaught receiver message type: ");
                    Serial.println((uint8_t)messageType);
                    incomingDataOffset = len;
                    break;
                }
            }

            shouldSendToReceiver = shouldSendToReceiver || (receiverMessageMap.size() > 0);
        }
    }
    void onEspNowDataSent(const uint8_t *macAddress, esp_now_send_status_t status)
    {
        Serial.print("\r\nLast Packet Send Status:\t");

        bool _isConnectedToReceiver;
        if (status == ESP_NOW_SEND_SUCCESS)
        {
            Serial.println("Delivery Success");
            _isConnectedToReceiver = true;
        }
        else
        {
            Serial.print("Delivery Failed: ");
            Serial.println(status);
            _isConnectedToReceiver = false;
        }

        if (isConnectedToReceiver != _isConnectedToReceiver)
        {
            Serial.println(isConnectedToReceiver ? "Connected to receiver" : "Disconneted from receiver");
            isConnectedToReceiver = _isConnectedToReceiver;
        }
    }

    void setup()
    {
        WiFi.mode(WIFI_STA);

        int32_t channel = getWiFiChannel(WIFI_SSID);

        WiFi.printDiag(Serial); // Uncomment to verify channel number before
        esp_wifi_set_promiscuous(true);
        esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
        esp_wifi_set_promiscuous(false);
        WiFi.printDiag(Serial); // Uncomment to verify channel change after

        if (esp_now_init() != ESP_OK)
        {
            Serial.println("Error initializing ESP-NOW");
            return;
        }

        esp_now_register_recv_cb(onEspNowDataReceived);
        esp_now_register_send_cb(onEspNowDataSent);

        esp_now_peer_info_t espNowPeerInfo;
        memset(&espNowPeerInfo, 0, sizeof(espNowPeerInfo));
        memcpy(espNowPeerInfo.peer_addr, receiverMacAddress, MAC_ADDRESS_SIZE);
        espNowPeerInfo.encrypt = false;

        while (esp_now_add_peer(&espNowPeerInfo) != ESP_OK)
        {
            Serial.println("Failed to connect to receiver");
            delay(2000);
        }
        Serial.println("added receiver");
        isConnectedToReceiver = true;
    }

    void batteryLevelLoop()
    {
        if (currentMillis - previousBatteryLevelMillis >= batteryLevelInterval)
        {
            previousBatteryLevelMillis = currentMillis;
            uint8_t data[1]{battery::getBatteryLevel()};
            receiverMessageMap[MessageType::BATTERY_LEVEL].assign(data, data + sizeof(data));
            shouldSendToReceiver = true;
        }
    }

    void motionCalibrationLoop()
    {
        if (currentMillis - previousMotionCalibrationMillis >= motionCalibrationInterval)
        {
            previousMotionCalibrationMillis = currentMillis;
            receiverMessageMap[MessageType::MOTION_CALIBRATION].assign(motion::calibration, motion::calibration + sizeof(motion::calibration));
            shouldSendToReceiver = true;
        }
    }

    unsigned long previousPingMillis = 0;
    const long pingInterval = 2000;
    void pingLoop()
    {
        if (currentMillis - previousPingMillis >= pingInterval)
        {
            previousPingMillis = currentMillis;
            if (receiverMessageMap.size() == 0)
            {
                receiverMessageMap[MessageType::PING];
                shouldSendToReceiver = true;
            }
        }
    }

    void sendLoop()
    {
        if (shouldSendToReceiver)
        {
            std::vector<uint8_t> receiverMessageData;
            for (auto receiverMessageIterator = receiverMessageMap.begin(); receiverMessageIterator != receiverMessageMap.end(); receiverMessageIterator++)
            {
                receiverMessageData.push_back((uint8_t)receiverMessageIterator->first);
                receiverMessageData.insert(receiverMessageData.end(), receiverMessageIterator->second.begin(), receiverMessageIterator->second.end());
            }

            Serial.print("Sending to Receiver: ");
            for (uint8_t index = 0; index < receiverMessageData.size(); index++)
            {
                Serial.print(receiverMessageData[index]);
                Serial.print(',');
            }
            Serial.println();

            esp_err_t result = esp_now_send(receiverMacAddress, receiverMessageData.data(), receiverMessageData.size());
            if (result == ESP_OK)
            {
                Serial.println("Delivery Success");
            }
            else
            {
                Serial.print("Delivery Failed: ");
                Serial.println(esp_err_to_name(result));
            }

            receiverMessageMap.clear();
            shouldSendToReceiver = false;
        }
    }

    void loop()
    {
        currentMillis = millis();
        if (isConnectedToClient())
        {
            lastTimeConnected = currentMillis;
        }
        batteryLevelLoop();
        motionCalibrationLoop();
        pingLoop();
        sendLoop();
    }
#endif
} // namespace wifiServer