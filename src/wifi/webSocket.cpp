#include "definitions.h"
#include "wifiServer.h"
#include "webSocket.h"

#include "ble/ble.h"

#include "information/name.h"
#include "information/type.h"
#include "debug.h"
#include "sensor/sensorData.h"
#include "sensor/motionSensor.h"
#include "battery.h"

namespace webSocket
{
    enum class MessageType : uint8_t
    {
        GET_DEBUG,
        SET_DEBUG,

        GET_TYPE,
        SET_TYPE,

        GET_NAME,
        SET_NAME,

        MOTION_CALIBRATION,

        GET_SENSOR_DATA_CONFIGURATIONS,
        SET_SENSOR_DATA_CONFIGURATIONS,

        SENSOR_DATA,

        BATTERY_LEVEL
    };

    AsyncWebSocket server("/ws");
    AsyncWebSocketClient *client = nullptr;
    bool isConnectedToClient()
    {
        return server.count() > 0 && client != nullptr;
    }

    std::map<MessageType, bool> _clientMessageFlags;
    bool shouldSendToClient = false;
    bool sentInitialPayload = false;
    void _onClientConnection()
    {
        Serial.println("Connected to websocket client");
        ble::pServer->stopAdvertising();
        sentInitialPayload = false;
    }
    void _onClientDisconnection()
    {
        Serial.println("Disconnected from websocket client");
        _clientMessageFlags.clear();
        ble::pServer->startAdvertising();
        sensorData::clearConfigurations();
        sentInitialPayload = false;
    }

    uint8_t onClientRequestGetDebug(uint8_t *data, uint8_t dataOffset)
    {
        if (_clientMessageFlags.count(MessageType::SET_DEBUG) == 0)
        {
            _clientMessageFlags[MessageType::GET_DEBUG] = true;
        }
        return dataOffset;
    }
    uint8_t onClientRequestSetDebug(uint8_t *data, uint8_t dataOffset)
    {
        auto enableDebug = (bool)data[dataOffset++];
        debug::setEnabled(enableDebug);
        _clientMessageFlags.erase(MessageType::GET_DEBUG);
        _clientMessageFlags[MessageType::SET_DEBUG] = true;
        return dataOffset;
    }
    uint8_t onClientRequestGetType(uint8_t *data, uint8_t dataOffset)
    {
        if (_clientMessageFlags.count(MessageType::SET_TYPE) == 0)
        {
            _clientMessageFlags[MessageType::GET_TYPE] = true;
        }
        return dataOffset;
    }
    uint8_t onClientRequestSetType(uint8_t *data, uint8_t dataOffset)
    {
        auto newType = (type::Type)data[dataOffset++];
        type::setType(newType);
        _clientMessageFlags.erase(MessageType::GET_TYPE);
        _clientMessageFlags[MessageType::SET_TYPE] = true;
        return dataOffset;
    }
    uint8_t onClientRequestGetName(uint8_t *data, uint8_t dataOffset)
    {
        if (_clientMessageFlags.count(MessageType::SET_NAME) == 0)
        {
            _clientMessageFlags[MessageType::GET_NAME] = true;
        }
        return dataOffset;
    }
    uint8_t onClientRequestSetName(uint8_t *data, uint8_t dataOffset)
    {
        auto nameLength = data[dataOffset++];
        auto newName = (char *)&data[dataOffset];
        dataOffset += nameLength;
        name::setName(newName, nameLength);

        _clientMessageFlags.erase(MessageType::GET_NAME);
        _clientMessageFlags[MessageType::SET_NAME] = true;
        return dataOffset;
    }
    uint8_t onClientRequestGetSensorDataConfigurations(uint8_t *data, uint8_t dataOffset)
    {
        if (_clientMessageFlags.count(MessageType::SET_SENSOR_DATA_CONFIGURATIONS) == 0)
        {
            _clientMessageFlags[MessageType::GET_SENSOR_DATA_CONFIGURATIONS] = true;
        }
        return dataOffset;
    }
    uint8_t onClientRequestSetSensorDataConfigurations(uint8_t *data, uint8_t dataOffset)
    {
        auto size = data[dataOffset++];
        sensorData::setConfigurations(&data[dataOffset], size);
        dataOffset += size;
        _clientMessageFlags.erase(MessageType::GET_SENSOR_DATA_CONFIGURATIONS);
        _clientMessageFlags[MessageType::SET_SENSOR_DATA_CONFIGURATIONS] = true;
        return dataOffset;
    }
    void _onWebSocketMessage(AsyncWebSocketClient *_client, void *arg, uint8_t *data, size_t len)
    {
        if (client == _client)
        {
            auto info = (AwsFrameInfo *)arg;
            if (info->final && info->index == 0 && info->len == len)
            {
#if DEBUG
                Serial.print("[WebSocket] message: ");
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
                    Serial.print("[WebSocket] message type: ");
                    Serial.println((uint8_t)messageType);
#endif

                    switch (messageType)
                    {
                    case MessageType::GET_DEBUG:
                        dataOffset = onClientRequestGetDebug(data, dataOffset);
                        break;
                    case MessageType::SET_DEBUG:
                        dataOffset = onClientRequestSetDebug(data, dataOffset);
                        break;
                    case MessageType::GET_TYPE:
                        dataOffset = onClientRequestGetType(data, dataOffset);
                        break;
                    case MessageType::SET_TYPE:
                        dataOffset = onClientRequestSetType(data, dataOffset);
                        break;
                    case MessageType::GET_NAME:
                        dataOffset = onClientRequestGetName(data, dataOffset);
                        break;
                    case MessageType::SET_NAME:
                        dataOffset = onClientRequestSetName(data, dataOffset);
                        break;
                    case MessageType::GET_SENSOR_DATA_CONFIGURATIONS:
                        dataOffset = onClientRequestGetSensorDataConfigurations(data, dataOffset);
                        break;
                    case MessageType::SET_SENSOR_DATA_CONFIGURATIONS:
                        dataOffset = onClientRequestSetSensorDataConfigurations(data, dataOffset);
                        break;
                    default:
                        Serial.print("uncaught websocket message type: ");
                        Serial.println((uint8_t)messageType);
                        dataOffset = len;
                        break;
                    }
                }

                shouldSendToClient = shouldSendToClient || (_clientMessageFlags.size() > 0);
            }
        }
    }

    void _onWebSocketEvent(AsyncWebSocket *_server, AsyncWebSocketClient *_client, AwsEventType type, void *arg, uint8_t *data, size_t len)
    {
        switch (type)
        {
        case WS_EVT_CONNECT:
            Serial.printf("WebSocket client #%u connected from %s\n", _client->id(), _client->remoteIP().toString().c_str());
            Serial.printf("There are currently %u clients\n", server.count());
            if (client == nullptr)
            {
                client = _client;
                _onClientConnection();
            }
            break;
        case WS_EVT_DISCONNECT:
            Serial.printf("WebSocket client #%u disconnected\n", _client->id());
            Serial.printf("There are currently %u clients\n", server.count());

            if (client == _client)
            {
                client = nullptr;
                _onClientDisconnection();
            }
            break;
        case WS_EVT_DATA:
#if DEBUG
            Serial.printf("Message of size #%u\n", len);
#endif
            if (client == _client)
            {
                _onWebSocketMessage(_client, arg, data, len);
            }
            break;
        case WS_EVT_PONG:
        case WS_EVT_ERROR:
            break;
        }
    }

    void setup()
    {
        server.onEvent(_onWebSocketEvent);
        wifiServer::server.addHandler(&server);
    }

    unsigned long currentTime = 0;
    void batteryLevelLoop()
    {
        // FIX LATER
    }
    unsigned long lastCalibrationUpdateTime;
    void motionCalibrationLoop()
    {
        if (lastCalibrationUpdateTime != motionSensor::lastCalibrationUpdateTime)
        {
            lastCalibrationUpdateTime = motionSensor::lastCalibrationUpdateTime;
            _clientMessageFlags[MessageType::MOTION_CALIBRATION] = true;
            shouldSendToClient = true;
        }
    }
    unsigned long lastSensorDataUpdateTime;
    void sensorDataLoop()
    {
        if (lastSensorDataUpdateTime != sensorData::lastDataUpdateTime && (sensorData::motionDataSize + sensorData::pressureDataSize > 0))
        {
            lastSensorDataUpdateTime = sensorData::lastDataUpdateTime;
            _clientMessageFlags[MessageType::SENSOR_DATA] = true;
            shouldSendToClient = true;
        }
    }

    uint8_t _clientMessageData[7 + 1 + 1 + name::MAX__NAME_LENGTH + sizeof(motionSensor::calibration) + sizeof(sensorData::motionConfiguration) + sizeof(sensorData::pressureConfiguration) + 2 + sizeof(sensorData::motionData) + 2 + sizeof(sensorData::pressureData) + 1];
    uint8_t _clientMessageDataSize = 0;
    void webSocketLoop()
    {
        server.cleanupClients();

        if (shouldSendToClient)
        {
            _clientMessageDataSize = 0;

            for (auto clientMessageFlagIterator = _clientMessageFlags.begin(); clientMessageFlagIterator != _clientMessageFlags.end(); clientMessageFlagIterator++)
            {
                auto messageType = clientMessageFlagIterator->first;
                _clientMessageData[_clientMessageDataSize++] = (uint8_t)messageType;

                switch (messageType)
                {
                case MessageType::GET_DEBUG:
                case MessageType::SET_DEBUG:
                    _clientMessageData[_clientMessageDataSize++] = (uint8_t)debug::getEnabled();
                    break;
                case MessageType::GET_TYPE:
                case MessageType::SET_TYPE:
                    _clientMessageData[_clientMessageDataSize++] = (uint8_t)type::getType();
                    break;
                case MessageType::GET_NAME:
                case MessageType::SET_NAME:
                {
                    auto _name = name::getName();
                    _clientMessageData[_clientMessageDataSize++] = _name->length();
                    memcpy(&_clientMessageData[_clientMessageDataSize], _name->c_str(), _name->length());
                    _clientMessageDataSize += _name->length();
                }
                break;
                case MessageType::MOTION_CALIBRATION:
                    memcpy(&_clientMessageData[_clientMessageDataSize], motionSensor::calibration, sizeof(motionSensor::calibration));
                    _clientMessageDataSize += sizeof(motionSensor::calibration);
                    break;
                case MessageType::GET_SENSOR_DATA_CONFIGURATIONS:
                case MessageType::SET_SENSOR_DATA_CONFIGURATIONS:
                    memcpy(&_clientMessageData[_clientMessageDataSize], sensorData::motionConfiguration, sizeof(sensorData::motionConfiguration));
                    _clientMessageDataSize += sizeof(sensorData::motionConfiguration);
                    memcpy(&_clientMessageData[_clientMessageDataSize], sensorData::pressureConfiguration, sizeof(sensorData::pressureConfiguration));
                    _clientMessageDataSize += sizeof(sensorData::pressureConfiguration);
                    break;
                case MessageType::SENSOR_DATA:
                    {
                        uint16_t timestamp = (uint16_t)lastSensorDataUpdateTime;
                        MEMCPY(&_clientMessageData[_clientMessageDataSize], &timestamp, sizeof(timestamp));
                        _clientMessageDataSize += sizeof(timestamp);

                        _clientMessageData[_clientMessageDataSize++] = (uint8_t)sensorData::SensorType::MOTION;
                        _clientMessageData[_clientMessageDataSize++] = sensorData::motionDataSize;
                        memcpy(&_clientMessageData[_clientMessageDataSize], sensorData::motionData, sensorData::motionDataSize);
                        _clientMessageDataSize += sensorData::motionDataSize;

                        _clientMessageData[_clientMessageDataSize++] = (uint8_t)sensorData::SensorType::PRESSURE;
                        _clientMessageData[_clientMessageDataSize++] = sensorData::pressureDataSize;
                        memcpy(&_clientMessageData[_clientMessageDataSize], sensorData::pressureData, sensorData::pressureDataSize);
                        _clientMessageDataSize += sensorData::pressureDataSize;
                    }
                    break;
                case MessageType::BATTERY_LEVEL:
                    // FIX LATER
                    _clientMessageData[_clientMessageDataSize++] = 100;
                    break;
                default:
                    Serial.print("uncaught client message type: ");
                    Serial.println((uint8_t)messageType);
                    break;
                }
            }

#if DEBUG
            Serial.print("Sending to client message of size ");
            Serial.print(_clientMessageDataSize);
            Serial.print(": ");
            for (uint8_t index = 0; index < _clientMessageDataSize; index++)
            {
                Serial.print(_clientMessageData[index]);
                Serial.print(',');
            }
            Serial.println();
#endif

            //server.binaryAll(_clientMessageData, _clientMessageDataSize);
            server.binary(client->id(), _clientMessageData, _clientMessageDataSize);
            
            if (!sentInitialPayload) {
                sentInitialPayload = true;
            }
            shouldSendToClient = false;
            _clientMessageFlags.clear();
        }
    }
    void loop()
    {
        currentTime = millis();
        if (isConnectedToClient() && client->canSend())
        {
            if (sentInitialPayload) {
                batteryLevelLoop();
                motionCalibrationLoop();
                sensorDataLoop();
            }
            webSocketLoop();
        }
    }
} // namespace webSocket
