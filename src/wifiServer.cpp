#include "wifiServer.h"
#include "name.h"
#include "battery.h"
#include "motion.h"
#include "pressure.h"

#include <esp_now.h>
#include <esp_wifi.h>

#include "Peer.h"

namespace wifiServer
{
    const char *ssid = WIFI_SSID;
    const char *password = WIFI_PASSWORD;

    unsigned long currentMillis = 0;
    unsigned long lastTimeConnected = 0;

    bool sentClientNumberOfDevices = false;

    unsigned long previousBatteryLevelMillis = 0;
    const unsigned long batteryLevelInterval = 20000;

    unsigned long previousMotionCalibrationMillis = 0;
    const unsigned long motionCalibrationInterval = 1000;

    unsigned long previousDataMillis = 0;

    bool includeTimestampInClientMessage = false;

    DeviceType deviceType = IS_INSOLE ? (IS_RIGHT_INSOLE ? DeviceType::RIGHT_INSOLE : DeviceType::LEFT_INSOLE) : DeviceType::MOTION_MODULE;

    uint16_t motionConfiguration[(uint8_t)motion::DataType::COUNT]{0};
    std::vector<uint8_t> getMotionData()
    {
        std::vector<uint8_t> motionData;
        for (uint8_t dataTypeIndex = 0; dataTypeIndex < (uint8_t)motion::DataType::COUNT; dataTypeIndex++)
        {
            auto dataType = (motion::DataType)dataTypeIndex;

            if (motionConfiguration[dataTypeIndex] != 0 && previousDataMillis % motionConfiguration[dataTypeIndex] == 0)
            {
                int16_t data[4];
                uint8_t dataSize = 0;

                switch (dataType)
                {
                case motion::DataType::ACCELERATION:
                    motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_ACCELEROMETER, data);
                    dataSize = 6;
                    break;
                case motion::DataType::GRAVITY:
                    motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_GRAVITY, data);
                    dataSize = 6;
                    break;
                case motion::DataType::LINEAR_ACCELERATION:
                    motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_LINEARACCEL, data);
                    dataSize = 6;
                    break;
                case motion::DataType::ROTATION_RATE:
                    motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_GYROSCOPE, data);
                    dataSize = 6;
                    break;
                case motion::DataType::MAGNETOMETER:
                    motion::bno.getRawVectorData(Adafruit_BNO055::VECTOR_MAGNETOMETER, data);
                    dataSize = 6;
                    break;
                case motion::DataType::QUATERNION:
                    motion::bno.getRawQuatData(data);
                    dataSize = 8;
                    break;
                default:
                    Serial.print("uncaught motion data type: ");
                    Serial.println((uint8_t)dataType);
                    break;
                }

                if (dataSize > 0)
                {
                    motionData.push_back((uint8_t)dataType);
                    motionData.insert(motionData.end(), (uint8_t *)data, ((uint8_t *)data) + dataSize);
                }
            }
        }

#if DEBUG
        if (motionData.size() > 0)
        {
            Serial.print("MOTION DATA: ");
            for (auto iterator = motionData.begin(); iterator != motionData.end(); iterator++)
            {
                Serial.print(*iterator);
                Serial.print(',');
            }
            Serial.println();
        }
#endif

        return motionData;
    }

    bool areMacAddressesEqual(const uint8_t *a, const uint8_t *b)
    {
        bool areEqual = true;
        for (uint8_t i = 0; i < MAC_ADDRESS_SIZE && areEqual; i++)
        {
            areEqual = areEqual && (a[i] == b[i]);
        }
        return areEqual;
    }

#if IS_RECEIVER

    AsyncWebServer server(80);

    AsyncWebSocket webSocket("/ws");

    bool isConnectedToClient()
    {
        return webSocket.count() > 0;
    }

    uint8_t getNumberOfDevices()
    {
        return Peer::getNumberOfPeers() + 1;
    }

    std::map<MessageType, std::vector<uint8_t>> clientMessageMap;
    std::map<uint8_t, std::map<MessageType, std::vector<uint8_t>>> deviceClientMessageMaps;
    bool shouldSendToClient = false;

    void onClientConnection()
    {
        Peer::OnClientConnection();
    }
    void onClientDisconnection()
    {
        Peer::OnClientDisconnection();
        memset(&motionConfiguration, 0, sizeof(motionConfiguration));
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
            auto peer = Peer::getPeerByDeviceIndex(deviceIndex);
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
                auto peer = Peer::getPeerByDeviceIndex(deviceIndex);
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
                auto peer = Peer::getPeerByDeviceIndex(deviceIndex);
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
                auto peer = Peer::getPeerByDeviceIndex(deviceIndex);
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
    uint8_t onClientRequestGetMotionConfiguration(uint8_t *data, uint8_t dataOffset)
    {
        uint8_t deviceIndex = data[dataOffset++];
        if (deviceIndex == 0)
        {
            uint8_t data[1 + sizeof(motionConfiguration)];
            data[0] = (uint8_t)ErrorMessageType::NO_ERROR;
            memcpy(&data[1], motionConfiguration, sizeof(motionConfiguration));
            deviceClientMessageMaps[deviceIndex][MessageType::GET_MOTION_CONFIGURATION].assign(data, data + sizeof(data));
        }
        else
        {
            try
            {
                auto peer = Peer::getPeerByDeviceIndex(deviceIndex);
                if (peer->motion.didUpdateConfigurationAtLeastOnce)
                {
                    uint8_t data[1 + sizeof(peer->motion.configuration)];
                    data[0] = (uint8_t)ErrorMessageType::NO_ERROR;
                    memcpy(&data[1], peer->motion.configuration, sizeof(peer->motion.configuration));
                    deviceClientMessageMaps[deviceIndex][MessageType::GET_MOTION_CONFIGURATION].assign(data, data + sizeof(data));
                }
                else
                {
                    peer->messageMap[MessageType::GET_MOTION_CONFIGURATION];
                }
            }
            catch (const std::out_of_range &error)
            {
                uint8_t data[1];
                data[0] = (uint8_t)ErrorMessageType::DEVICE_NOT_FOUND;
                deviceClientMessageMaps[deviceIndex][MessageType::GET_MOTION_CONFIGURATION].assign(data, data + sizeof(data));
            }
        }

        return dataOffset;
    }
    uint8_t onClientRequestSetMotionConfiguration(uint8_t *data, uint8_t dataOffset)
    {
        uint8_t deviceIndex = data[dataOffset++];
        auto _motionConfiguration = (uint16_t *)&data[dataOffset];
        dataOffset += sizeof(motionConfiguration);
        if (deviceIndex == 0)
        {
            memcpy(motionConfiguration, _motionConfiguration, sizeof(motionConfiguration));
            uint8_t data[1 + sizeof(motionConfiguration)];
            data[0] = (uint8_t)ErrorMessageType::NO_ERROR;
            memcpy(&data[1], motionConfiguration, sizeof(motionConfiguration));
            deviceClientMessageMaps[deviceIndex][MessageType::GET_MOTION_CONFIGURATION].assign(data, data + sizeof(data));
        }
        else
        {
            try
            {
                auto peer = Peer::getPeerByDeviceIndex(deviceIndex);
                peer->messageMap[MessageType::SET_MOTION_CONFIGURATION].assign((uint8_t *)_motionConfiguration, (uint8_t *)_motionConfiguration + sizeof(motionConfiguration));
            }
            catch (const std::out_of_range &error)
            {
                uint8_t data[1];
                data[0] = (uint8_t)ErrorMessageType::DEVICE_NOT_FOUND;
                deviceClientMessageMaps[deviceIndex][MessageType::SET_MOTION_CONFIGURATION].assign(data, data + sizeof(data));
            }
        }

        return dataOffset;
    }

    void onWebSocketMessage(AsyncWebSocketClient *client, void *arg, uint8_t *data, size_t len)
    {
        auto info = (AwsFrameInfo *)arg;
        if (info->final && info->index == 0 && info->len == len)
        {
#if DEBUG
            Serial.print("WebSocket: ");
            for (uint8_t index = 0; index < len; index++)
            {
                Serial.print(data[index]);
                Serial.print(',');
            }
            Serial.println();
#endif

            uint8_t dataOffset = 0;
            MessageType messageType;
            while (dataOffset < len)
            {
                messageType = (MessageType)data[dataOffset++];
#if DEBUG
                Serial.print("Message Type from client: ");
                Serial.println((uint8_t)messageType);
#endif

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
                case MessageType::GET_MOTION_CONFIGURATION:
                    dataOffset = onClientRequestGetMotionConfiguration(data, dataOffset);
                    break;
                case MessageType::SET_MOTION_CONFIGURATION:
                    dataOffset = onClientRequestSetMotionConfiguration(data, dataOffset);
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
#if DEBUG
            Serial.printf("Message of size #%u\n", len);
#endif
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
#if DEBUG
                        Serial.print("INDEX: ");
                        Serial.println((uint8_t)deviceClientMessagesIterator->first);
#endif
                        for (auto deviceClientMessageIterator = deviceClientMessagesIterator->second.begin(); deviceClientMessageIterator != deviceClientMessagesIterator->second.end(); deviceClientMessageIterator++)
                        {
#if DEBUG
                            Serial.print("MESSAGE TYPE: ");
                            Serial.println((uint8_t)deviceClientMessageIterator->first);
#endif
                            clientMessageData.push_back((uint8_t)deviceClientMessageIterator->first);
                            clientMessageData.push_back(deviceClientMessagesIterator->first);
                            clientMessageData.insert(clientMessageData.end(), deviceClientMessageIterator->second.begin(), deviceClientMessageIterator->second.end());
                        }
                    }
                }

#if DEBUG
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
#endif

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
#if DEBUG
        char macAddressString[18];
        Serial.print("Packet received from: ");
        snprintf(macAddressString, sizeof(macAddressString), "%02x:%02x:%02x:%02x:%02x:%02x",
                 macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
        Serial.println(macAddressString);
#endif

        auto peer = Peer::getPeerByMacAddress(macAddress);
        if (peer == nullptr)
        {
            peer = new Peer(macAddress);
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
#if DEBUG
            Serial.println("already connected to peer");
#endif
        }

        peer->onMessage(incomingData, len);
    }
    void onEspNowDataSent(const uint8_t *macAddress, esp_now_send_status_t status)
    {
        auto peer = Peer::getPeerByMacAddress(macAddress);
        if (peer != nullptr)
        {
#if DEBUG
            Serial.print("\r\nLast Packet Send Status:\t");
#endif
            if (status == ESP_NOW_SEND_SUCCESS)
            {
#if DEBUG
                Serial.println("Delivery Success");
#endif
                peer->updateAvailability(true);
            }
            else
            {
#if DEBUG
                Serial.print("Delivery Failed: ");
                Serial.println(status);
#endif

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
            previousBatteryLevelMillis = currentMillis - (currentMillis % batteryLevelInterval);

            if (isConnectedToClient())
            {
                uint8_t data[1]{battery::getBatteryLevel()};
                deviceClientMessageMaps[0][MessageType::BATTERY_LEVEL].assign(data, data + sizeof(data));
                shouldSendToClient = true;

                Peer::BatteryLevelLoop();
            }
        }
    }

    void motionCalibrationLoop()
    {
        if (currentMillis - previousMotionCalibrationMillis >= motionCalibrationInterval)
        {
            previousMotionCalibrationMillis = currentMillis - (currentMillis % motionCalibrationInterval);

            deviceClientMessageMaps[0][MessageType::MOTION_CALIBRATION].assign(motion::calibration, motion::calibration + sizeof(motion::calibration));
            shouldSendToClient = true;

            Peer::MotionCalibrationLoop();
        }
    }

    void motionDataLoop()
    {
        auto motionData = getMotionData();
        if (motionData.size() > 0)
        {
            deviceClientMessageMaps[0][MessageType::MOTION_DATA].push_back(motionData.size());
            deviceClientMessageMaps[0][MessageType::MOTION_DATA].insert(deviceClientMessageMaps[0][MessageType::MOTION_DATA].end(), motionData.begin(), motionData.end());

            shouldSendToClient = true;
            includeTimestampInClientMessage = true;
        }

        Peer::MotionDataLoop();
    }

    void pressureDataLoop()
    {
        // fiLL
    }

    void dataLoop()
    {
        if (currentMillis - previousDataMillis >= dataInterval)
        {
            previousDataMillis = currentMillis - (currentMillis % dataInterval);

            motionDataLoop();
            pressureDataLoop();

            if (includeTimestampInClientMessage)
            {
                uint8_t timestamp[sizeof(currentMillis)];
                memcpy(timestamp, &currentMillis, sizeof(currentMillis));
                clientMessageMap[MessageType::TIMESTAMP].assign(timestamp, timestamp + sizeof(timestamp));

                includeTimestampInClientMessage = false;
            }
        }
    }

    void loop()
    {
        currentMillis = millis();

        if (sentClientNumberOfDevices)
        {
            batteryLevelLoop();
            motionCalibrationLoop();
            dataLoop();
        }
        Peer::pingLoop();
        Peer::sendAll();
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
    uint8_t onReceiverRequestGetName(const uint8_t *incomingData, uint8_t incomingDataOffset)
    {
        auto nameString = name::getName();

        std::vector<uint8_t> data;
        data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
        data.push_back(nameString->length());
        data.insert(data.end(), nameString->begin(), nameString->end());
        if (receiverMessageMap.count(MessageType::SET_NAME) == 0)
        {
            receiverMessageMap[MessageType::GET_NAME] = data;
        }

        return incomingDataOffset;
    }
    uint8_t onReceiverRequestSetName(const uint8_t *incomingData, uint8_t incomingDataOffset)
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
        receiverMessageMap[MessageType::SET_NAME] = data;

        return incomingDataOffset;
    }
    uint8_t onReceiverRequestGetType(const uint8_t *incomingData, uint8_t incomingDataOffset)
    {
        uint8_t data[2];
        data[0] = (uint8_t)ErrorMessageType::NO_ERROR;
        data[1] = (uint8_t)deviceType;
        receiverMessageMap[MessageType::GET_TYPE].assign(data, data + sizeof(data));
        return incomingDataOffset;
    }
    uint8_t onReceiverRequestGetMotionConfiguration(const uint8_t *incomingData, uint8_t incomingDataOffset)
    {
        uint8_t data[1 + sizeof(motionConfiguration)];
        data[0] = (uint8_t)ErrorMessageType::NO_ERROR;
        memcpy(&data[1], motionConfiguration, sizeof(motionConfiguration));
        receiverMessageMap[MessageType::GET_MOTION_CONFIGURATION].assign(data, data + sizeof(data));
        return incomingDataOffset;
    }
    uint8_t onReceiverRequestSetMotionConfiguration(const uint8_t *incomingData, uint8_t incomingDataOffset)
    {
        auto _motionConfiguration = (uint16_t *)&incomingData[incomingDataOffset];
        incomingDataOffset += sizeof(motionConfiguration);
        memcpy(motionConfiguration, _motionConfiguration, sizeof(motionConfiguration));

        uint8_t data[1 + sizeof(motionConfiguration)];
        data[0] = (uint8_t)ErrorMessageType::NO_ERROR;
        memcpy(&data[1], motionConfiguration, sizeof(motionConfiguration));
        receiverMessageMap[MessageType::SET_MOTION_CONFIGURATION].assign(data, data + sizeof(data));
        return incomingDataOffset;
    }
    void onEspNowDataReceived(const uint8_t *macAddress, const uint8_t *incomingData, int len)
    {
#if DEBUG
        char macAddressString[18];
        Serial.print("Packet received from: ");
        snprintf(macAddressString, sizeof(macAddressString), "%02x:%02x:%02x:%02x:%02x:%02x",
                 macAddress[0], macAddress[1], macAddress[2], macAddress[3], macAddress[4], macAddress[5]);
        Serial.println(macAddressString);
#endif

        if (areMacAddressesEqual(macAddress, receiverMacAddress))
        {
#if DEBUG
            Serial.print("EspNowReceiver: ");
            for (uint8_t index = 0; index < len; index++)
            {
                Serial.print(incomingData[index]);
                Serial.print(',');
            }
            Serial.println();
#endif

            uint8_t incomingDataOffset = 0;
            MessageType messageType;
            while (incomingDataOffset < len)
            {
                messageType = (MessageType)incomingData[incomingDataOffset++];

#if DEBUG
                Serial.print("Message Type from receiver: ");
                Serial.println((uint8_t)messageType);
#endif

                switch (messageType)
                {
                case MessageType::GET_NAME:
                    incomingDataOffset = onReceiverRequestGetName(incomingData, incomingDataOffset);
                    break;
                case MessageType::SET_NAME:
                    incomingDataOffset = onReceiverRequestSetName(incomingData, incomingDataOffset);
                    break;
                case MessageType::GET_TYPE:
                    incomingDataOffset = onReceiverRequestGetType(incomingData, incomingDataOffset);
                    break;
                case MessageType::GET_MOTION_CONFIGURATION:
                    incomingDataOffset = onReceiverRequestGetMotionConfiguration(incomingData, incomingDataOffset);
                    break;
                case MessageType::SET_MOTION_CONFIGURATION:
                    incomingDataOffset = onReceiverRequestSetMotionConfiguration(incomingData, incomingDataOffset);
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
                    memset(&motionConfiguration, 0, sizeof(motionConfiguration));
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
#if DEBUG
        Serial.print("\r\nLast Packet Send Status:\t");
#endif

        bool _isConnectedToReceiver;
        if (status == ESP_NOW_SEND_SUCCESS)
        {
#if DEBUG
            Serial.println("Delivery Success");
#endif
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
            previousBatteryLevelMillis = currentMillis - (currentMillis % batteryLevelInterval);

            uint8_t data[1]{battery::getBatteryLevel()};
            receiverMessageMap[MessageType::BATTERY_LEVEL].assign(data, data + sizeof(data));
            shouldSendToReceiver = true;
        }
    }

    void motionCalibrationLoop()
    {
        if (currentMillis - previousMotionCalibrationMillis >= motionCalibrationInterval)
        {
            previousMotionCalibrationMillis = currentMillis - (currentMillis % motionCalibrationInterval);

            receiverMessageMap[MessageType::MOTION_CALIBRATION].assign(motion::calibration, motion::calibration + sizeof(motion::calibration));
            shouldSendToReceiver = true;
        }
    }

    void motionDataLoop()
    {
        auto motionData = getMotionData();
        if (motionData.size() > 0)
        {
            receiverMessageMap[MessageType::MOTION_DATA].push_back(motionData.size());
            receiverMessageMap[MessageType::MOTION_DATA].insert(receiverMessageMap[MessageType::MOTION_DATA].end(), motionData.begin(), motionData.end());

            includeTimestampInClientMessage = true;
            shouldSendToReceiver = true;
        }
    }

    void pressureDataLoop()
    {
        // fiLL
    }

    void dataLoop()
    {
        if (currentMillis - previousDataMillis >= dataInterval)
        {
            previousDataMillis = currentMillis - (currentMillis % dataInterval);

            motionDataLoop();
            pressureDataLoop();

            if (includeTimestampInClientMessage)
            {
                uint8_t timestamp[sizeof(currentMillis)];
                memcpy(timestamp, &currentMillis, sizeof(currentMillis));
                receiverMessageMap[MessageType::TIMESTAMP].assign(timestamp, timestamp + sizeof(timestamp));

                includeTimestampInClientMessage = false;
            }
        }
    }

    unsigned long previousPingMillis = 0;
    const unsigned long pingInterval = 2000;
    void pingLoop()
    {
        if (currentMillis - previousPingMillis >= pingInterval)
        {
            previousPingMillis = currentMillis - (currentMillis % pingInterval);
            if (!shouldSendToReceiver)
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

#if DEBUG
            Serial.print("Sending to Receiver: ");
            for (uint8_t index = 0; index < receiverMessageData.size(); index++)
            {
                Serial.print(receiverMessageData[index]);
                Serial.print(',');
            }
            Serial.println();
#endif

            esp_err_t result = esp_now_send(receiverMacAddress, receiverMessageData.data(), receiverMessageData.size());
            if (result == ESP_OK)
            {
#if DEBUG
                Serial.println("Delivery Success");
#endif
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
        dataLoop();
        pingLoop();
        sendLoop();
    }
#endif
} // namespace wifiServer