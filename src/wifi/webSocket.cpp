#include "definitions.h"
#include "wifiServer.h"
#include "webSocket.h"

#include "ble/ble.h"

#include "information/name.h"
#include "information/type.h"
#include "sensor/sensorData.h"
#include "sensor/motionSensor.h"
#include "weight/weightData.h"
#include "battery.h"

#include <FS.h>
#ifdef USE_LittleFS
#define SPIFFS LITTLEFS
#include <LITTLEFS.h>
#else
#include <SPIFFS.h>
#endif

#include <Update.h>

namespace webSocket
{
    constexpr uint16_t max_message_size = 2930;

    enum class MessageType : uint8_t
    {
        BATTERY_LEVEL,

        GET_TYPE,
        SET_TYPE,

        GET_NAME,
        SET_NAME,

        MOTION_CALIBRATION,

        GET_SENSOR_DATA_CONFIGURATIONS,
        SET_SENSOR_DATA_CONFIGURATIONS,

        SENSOR_DATA,

        GET_WEIGHT_DATA_DELAY,
        SET_WEIGHT_DATA_DELAY,

        WEIGHT_DATA,

        RECEIVE_FILE,
        SEND_FILE,
        REMOVE_FILE,
        FORMAT_FILESYSTEM,

        GET_FIRMWARE_VERSION,
        FIRMWARE_UPDATE
    };

    AsyncWebSocket server("/ws");
    AsyncWebSocketClient *client = nullptr;
    bool isConnectedToClient()
    {
        return server.count() > 0 && client != nullptr;
    }

    enum class FileType : uint8_t
    {
        NONE,
        FIRMWARE,
        FILE
    };
    FileType incomingFileType = FileType::NONE;

    File file;
    std::string filePath = "";
    uint32_t fileSize = 0;
    uint32_t bytesTransferred = 0;

    void removeFile()
    {
        Serial.println("removing file...");

        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }

        if (SPIFFS.exists(filePath.c_str()))
        {
            SPIFFS.remove(filePath.c_str());
            Serial.println("removed file");
        }
        else
        {
            Serial.println("file doesn't exist");
        }
        SPIFFS.end();
    }
    void formatFilesystem()
    {
        Serial.println("formatting filesystem...");

        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return;
        }
        SPIFFS.format();
        SPIFFS.end();
        Serial.println("formatted filesystem");
    }

    bool shouldSendFile = false;
    uint8_t fileBuffer[max_message_size];
    void sendFilePacket() {
        auto bytesLeftToRead = fileSize - bytesTransferred;
        auto bytesToSend = (bytesLeftToRead > max_message_size-1) ? max_message_size-1 : bytesLeftToRead;
        fileBuffer[0] = (uint8_t) MessageType::SEND_FILE;
        file.read(&fileBuffer[1], bytesToSend);
        bytesTransferred += bytesToSend;

        Serial.printf("sent %u bytes of %u\n", bytesTransferred, fileSize);

        server.binary(client->id(), fileBuffer, bytesToSend+1);

        if (bytesTransferred >= fileSize)
        {
            Serial.println("finished sending file!");
            shouldSendFile = false;
            file.close();
            SPIFFS.end();
        }
    }

    std::map<MessageType, bool> _clientMessageFlags;
    bool shouldSendToClient = false;
    bool sentInitialPayload = false;
    void _onClientConnection()
    {
        Serial.println("Connected to websocket client");
        ble::pServer->stopAdvertising();
        sentInitialPayload = false;
        server.enable(false);
    }
    void _onClientDisconnection()
    {
        Serial.println("Disconnected from websocket client");
        _clientMessageFlags.clear();
        ble::pServer->startAdvertising();
        sensorData::clearConfigurations();
        weightData::setDelay(0);
        sentInitialPayload = false;
        server.enable(true);
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
    uint8_t onClientRequestGetWeightDataDelay(uint8_t *data, uint8_t dataOffset)
    {
        if (_clientMessageFlags.count(MessageType::SET_WEIGHT_DATA_DELAY) == 0)
        {
            _clientMessageFlags[MessageType::GET_WEIGHT_DATA_DELAY] = true;
        }
        return dataOffset;
    }
    uint8_t onClientRequestSetWeightDataDelay(uint8_t *data, uint8_t dataOffset)
    {
        uint16_t delay = (((uint16_t)data[dataOffset + 1]) << 8) | ((uint16_t)data[dataOffset]);
        dataOffset += sizeof(delay);
        weightData::setDelay(delay);
        _clientMessageFlags.erase(MessageType::GET_WEIGHT_DATA_DELAY);
        _clientMessageFlags[MessageType::SET_WEIGHT_DATA_DELAY] = true;
        return dataOffset;
    }
    uint8_t onClientRequestReceiveFile(uint8_t *data, uint8_t dataOffset)
    {
        fileSize = (((uint32_t)data[dataOffset + 3]) << 24) | (((uint32_t)data[dataOffset + 2]) << 16) | ((uint32_t)data[dataOffset + 1]) << 8 | ((uint32_t)data[dataOffset]);
        bytesTransferred = 0;
        dataOffset += 4;
        auto filePathLength = data[dataOffset++];
        filePath.assign((char *)&data[dataOffset], filePathLength);
        dataOffset += filePathLength;
        Serial.printf("anticipating %s of size %u\n", filePath.c_str(), fileSize);

        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return dataOffset;
        }
        file = SPIFFS.open(filePath.c_str(), FILE_WRITE);

        if (incomingFileType == FileType::NONE)
        {
            incomingFileType = FileType::FILE;
        }
        return dataOffset;
    }
    uint8_t onClientRequestSendFile(uint8_t *data, uint8_t dataOffset)
    {
        auto filePathLength = data[dataOffset++];
        filePath.assign((char *)&data[dataOffset], filePathLength);
        dataOffset += filePathLength;
        Serial.printf("request to send %s\n", filePath.c_str());

        if (!SPIFFS.begin(FORMAT_SPIFFS_IF_FAILED))
        {
            Serial.println("SPIFFS Mount Failed");
            return dataOffset;
        }
        if (SPIFFS.exists(filePath.c_str()))
        {
            file = SPIFFS.open(filePath.c_str(), FILE_READ);
            fileSize = file.size();
        }
        else
        {
            fileSize = 0;
        }

        bytesTransferred = 0;

        _clientMessageFlags[MessageType::SEND_FILE] = true;
        shouldSendToClient = true;
        return dataOffset;
    }
    uint8_t onClientRequestRemoveFile(uint8_t *data, uint8_t dataOffset)
    {
        auto filePathLength = data[dataOffset++];
        filePath.assign((char *)&data[dataOffset], filePathLength);
        dataOffset += filePathLength;
        Serial.printf("request to remove %s\n", filePath.c_str());
        _clientMessageFlags[MessageType::REMOVE_FILE] = true;
        return dataOffset;
    }
    uint8_t onClientRequestFormatFilesystem(uint8_t *data, uint8_t dataOffset)
    {
        _clientMessageFlags[MessageType::FORMAT_FILESYSTEM] = true;
        return dataOffset;
    }
    uint8_t onClientRequestGetFirmwareVersion(uint8_t *data, uint8_t dataOffset)
    {
        if (_clientMessageFlags.count(MessageType::GET_FIRMWARE_VERSION) == 0)
        {
            _clientMessageFlags[MessageType::GET_FIRMWARE_VERSION] = true;
        }
        return dataOffset;
    }
    uint8_t onClientRequestFirmwareUpdate(uint8_t *data, uint8_t dataOffset)
    {
        uint32_t firmwareSize = (((uint32_t)data[dataOffset + 3]) << 24) | (((uint32_t)data[dataOffset + 2]) << 16) | ((uint32_t)data[dataOffset + 1]) << 8 | ((uint32_t)data[dataOffset]);
        dataOffset += sizeof(firmwareSize);

        Serial.printf("firmware size: %u\n", firmwareSize);

        if (incomingFileType == FileType::NONE)
        {
            if (Update.isRunning())
            {
                Update.abort();
            }

            if (Update.begin(firmwareSize))
            {
                incomingFileType = FileType::FIRMWARE;
            }
            else
            {
                Update.printError(Serial);
            }
        }

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
                    case MessageType::GET_WEIGHT_DATA_DELAY:
                        dataOffset = onClientRequestGetWeightDataDelay(data, dataOffset);
                        break;
                    case MessageType::SET_WEIGHT_DATA_DELAY:
                        dataOffset = onClientRequestSetWeightDataDelay(data, dataOffset);
                        break;
                    case MessageType::RECEIVE_FILE:
                        dataOffset = onClientRequestReceiveFile(data, dataOffset);
                        break;
                    case MessageType::SEND_FILE:
                        dataOffset = onClientRequestSendFile(data, dataOffset);
                        break;
                    case MessageType::REMOVE_FILE:
                        dataOffset = onClientRequestRemoveFile(data, dataOffset);
                        break;
                    case MessageType::FORMAT_FILESYSTEM:
                        dataOffset = onClientRequestFormatFilesystem(data, dataOffset);
                        break;
                    case MessageType::GET_FIRMWARE_VERSION:
                        dataOffset = onClientRequestGetFirmwareVersion(data, dataOffset);
                        break;
                    case MessageType::FIRMWARE_UPDATE:
                        dataOffset = onClientRequestFirmwareUpdate(data, dataOffset);
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
            else
            {
                if (info->index == 0)
                {
                    if (info->num == 0)
                    {
#if DEBUG
                        Serial.printf("ws[%s][%u] %s-message start\n", server.url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
#endif
                    }
#if DEBUG
                    Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server.url(), client->id(), info->num, info->len);
#endif
                }

#if DEBUG
                Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server.url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);
#endif

                switch (incomingFileType)
                {
                case FileType::FIRMWARE:
                    Update.write(data, len);
                    break;
                case FileType::FILE:
                    Serial.printf("received payload of size %u\n", len);
                    file.write(data, len);
                    bytesTransferred += len;
                    Serial.printf("received %d/%d bytes\n", bytesTransferred, fileSize);
                    break;
                default:
                    break;
                }

                if ((info->index + len) == info->len)
                {
#if DEBUG
                    Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server.url(), client->id(), info->num, info->len);
#endif
                    if (info->final)
                    {
#if DEBUG
                        Serial.printf("ws[%s][%u] %s-message end\n", server.url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
#endif

                        switch (incomingFileType)
                        {
                        case FileType::FIRMWARE:
                            if (Update.isRunning())
                            {
                                Serial.println("OTA done!");
                                if (Update.end(true))
                                {
                                    if (Update.isFinished())
                                    {
                                        Serial.println("Update successfully completed. Rebooting.");
                                        ESP.restart();
                                    }
                                    else
                                    {
                                        Serial.println("Update not finished? Something went wrong!");
                                    }
                                }
                                else
                                {
                                    Update.printError(Serial);
                                }
                            }
                            break;
                        case FileType::FILE:
                            Serial.println("finished file!");
                            Serial.printf("received %d/%d bytes!!\n", bytesTransferred, fileSize);
                            file.close();
                            SPIFFS.end();
                            _clientMessageFlags[MessageType::RECEIVE_FILE] = true;
                            shouldSendToClient = true;
                            break;
                        default:
                            break;
                        }
                    }
                }
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

    unsigned long lastWeightDataUpdateTime;
    void weightDataLoop()
    {
        if (lastWeightDataUpdateTime != weightData::lastDataUpdateTime)
        {
            lastWeightDataUpdateTime = weightData::lastDataUpdateTime;
            _clientMessageFlags[MessageType::WEIGHT_DATA] = true;
            shouldSendToClient = true;
        }
    }

    // [...[messageType, payloadSize?, payload]], with payloadSize for variable payloads (name) excluding sensorData
    // sensorData is [messageType, timestamp, motionPayloadSize, motionPayload, pressurePayloadSize, pressurePayload]
    uint8_t _clientMessageData[1 + sizeof(uint8_t) + 1 + sizeof(bool) + 1 + sizeof(type::Type) + 2 + name::MAX_NAME_LENGTH + 1 + sizeof(motionSensor::calibration) + 1 + (sizeof(uint16_t) * sensorData::configurations.flattened.max_size()) + 1 + 1 + sizeof(uint16_t) + 2 + sizeof(sensorData::motionData) + 2 + sizeof(sensorData::pressureData) + 1 + sizeof(uint16_t) + 1 + sizeof(float)];
    uint8_t _clientMessageDataSize = 0;

    unsigned long lastTimeServerCleanedUpClients = 0;
    const uint16_t cleanup_clients_delay_ms = 1000;
    void cleanupClientsLoop()
    {
        if (currentTime >= lastTimeServerCleanedUpClients + cleanup_clients_delay_ms)
        {
            lastTimeServerCleanedUpClients = currentTime - (currentTime % cleanup_clients_delay_ms);
            server.cleanupClients();
        }
    }

    unsigned long lastSendFileTime = 0;
    uint16_t send_file_delay_ms = 20;
    void sendFileLoop() {
        if (shouldSendFile && currentTime >= lastSendFileTime + send_file_delay_ms)
        {
            lastSendFileTime = currentTime - (currentTime % send_file_delay_ms);
            sendFilePacket();
        }
    }

    std::string firmwareVersion = "0.0";
    void messageLoop()
    {
        if (shouldSendToClient)
        {
            _clientMessageDataSize = 0;
            
            for (auto clientMessageFlagIterator = _clientMessageFlags.begin(); clientMessageFlagIterator != _clientMessageFlags.end(); clientMessageFlagIterator++)
            {
                auto messageType = clientMessageFlagIterator->first;
                _clientMessageData[_clientMessageDataSize++] = (uint8_t)messageType;

                switch (messageType)
                {
                case MessageType::BATTERY_LEVEL:
                    // FIX LATER
                    _clientMessageData[_clientMessageDataSize++] = 100;
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
                    memcpy(&_clientMessageData[_clientMessageDataSize], (uint8_t *)sensorData::configurations.flattened.data(), sizeof(uint16_t) * sensorData::configurations.flattened.max_size());
                    _clientMessageDataSize += sizeof(uint16_t) * sensorData::configurations.flattened.max_size();
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
                case MessageType::GET_WEIGHT_DATA_DELAY:
                case MessageType::SET_WEIGHT_DATA_DELAY:
                {
                    auto delay = weightData::getDelay();
                    memcpy(&_clientMessageData[_clientMessageDataSize], (uint8_t *)&delay, sizeof(delay));
                    _clientMessageDataSize += sizeof(delay);
                }
                break;
                case MessageType::WEIGHT_DATA:
                {
                    auto weight = weightData::getWeight();
                    memcpy(&_clientMessageData[_clientMessageDataSize], (uint8_t *)&weight, sizeof(weight));
                    _clientMessageDataSize += sizeof(weight);
                }
                break;
                case MessageType::RECEIVE_FILE:
                    _clientMessageData[_clientMessageDataSize++] = filePath.length();
                    memcpy(&_clientMessageData[_clientMessageDataSize], filePath.c_str(), filePath.length());
                    _clientMessageDataSize += filePath.length();
                    break;
                case MessageType::SEND_FILE:
                    _clientMessageData[_clientMessageDataSize++] = filePath.length();
                    memcpy(&_clientMessageData[_clientMessageDataSize], filePath.c_str(), filePath.length());
                    _clientMessageDataSize += filePath.length();
                    memcpy(&_clientMessageData[_clientMessageDataSize], (uint8_t *)&fileSize, sizeof(fileSize));
                    _clientMessageDataSize += sizeof(fileSize);
                    shouldSendFile = true;
                    break;
                case MessageType::REMOVE_FILE:
                    removeFile();
                    _clientMessageData[_clientMessageDataSize++] = filePath.length();
                    memcpy(&_clientMessageData[_clientMessageDataSize], filePath.c_str(), filePath.length());
                    _clientMessageDataSize += filePath.length();
                    break;
                case MessageType::FORMAT_FILESYSTEM:
                    formatFilesystem();
                    break;
                case MessageType::GET_FIRMWARE_VERSION:
                    _clientMessageData[_clientMessageDataSize++] = firmwareVersion.length();
                    memcpy(&_clientMessageData[_clientMessageDataSize], firmwareVersion.c_str(), firmwareVersion.length());
                    _clientMessageDataSize += firmwareVersion.length();
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

            server.binary(client->id(), _clientMessageData, _clientMessageDataSize);

            if (!sentInitialPayload)
            {
                sentInitialPayload = true;
            }
            shouldSendToClient = false;
            _clientMessageFlags.clear();
        }
    }
    void loop()
    {
        currentTime = millis();

        cleanupClientsLoop();

        if (isConnectedToClient() && client->canSend())
        {
            if (sentInitialPayload)
            {
                if (!shouldSendFile) {
                    batteryLevelLoop();
                    // motionCalibrationLoop();
                    sensorDataLoop();
                    weightDataLoop();
                }
                else {
                    sendFileLoop();
                }
            }
            messageLoop();
        }
    }
} // namespace webSocket
