#include "definitions.h"
#include "wifiServer.h"
#include "ble/ble.h"
#include "udp.h"
#include "information/type.h"
#include "information/name.h"
#include "sensor/sensorData.h"

namespace udp
{
    enum class MessageType : uint8_t
    {
        PING,

        BATTERY_LEVEL,

        GET_TYPE,
        SET_TYPE,

        GET_NAME,
        SET_NAME,

        MOTION_CALIBRATION,

        GET_SENSOR_DATA_CONFIGURATIONS,
        SET_SENSOR_DATA_CONFIGURATIONS,

        SENSOR_DATA
    };

    AsyncUDP udp;
    unsigned long currentTime = 0;

    bool _hasListener = false;
    bool hasListener()
    {
        return _hasListener;
    }
    std::map<MessageType, bool> _listenerMessageFlags;
    bool shouldSendToListener = false;

    IPAddress remoteIP;
    uint16_t remotePort;
    unsigned long lastTimePacketWasReceivedByListener;

    void _onListenerConnection()
    {
        ble::pServer->stopAdvertising();
        Serial.println("[udp] listener connected");
    }
    void _onListenerDisconnection()
    {
        ble::pServer->startAdvertising();
        Serial.println("[udp] listener disconnected");
        sensorData::clearConfigurations();
    }

    uint8_t onListenerRequestGetType(uint8_t *data, uint8_t dataOffset)
    {
        if (_listenerMessageFlags.count(MessageType::SET_TYPE) == 0)
        {
            _listenerMessageFlags[MessageType::GET_TYPE] = true;
        }
        return dataOffset;
    }
    uint8_t onListenerRequestSetType(uint8_t *data, uint8_t dataOffset)
    {
        auto newType = (type::Type)data[dataOffset++];
        type::setType(newType);
        _listenerMessageFlags.erase(MessageType::GET_TYPE);
        _listenerMessageFlags[MessageType::SET_TYPE] = true;
        return dataOffset;
    }
    int8_t onListenerRequestGetName(uint8_t *data, uint8_t dataOffset)
    {
        if (_listenerMessageFlags.count(MessageType::SET_NAME) == 0)
        {
            _listenerMessageFlags[MessageType::GET_NAME] = true;
        }
        return dataOffset;
    }
    uint8_t onListenerRequestSetName(uint8_t *data, uint8_t dataOffset)
    {
        auto nameLength = data[dataOffset++];
        auto newName = (char *)&data[dataOffset];
        dataOffset += nameLength;
        name::setName(newName, nameLength);

        _listenerMessageFlags.erase(MessageType::GET_NAME);
        _listenerMessageFlags[MessageType::SET_NAME] = true;
        return dataOffset;
    }
    uint8_t onListenerRequestGetSensorDataConfigurations(uint8_t *data, uint8_t dataOffset)
    {
        if (_listenerMessageFlags.count(MessageType::SET_SENSOR_DATA_CONFIGURATIONS) == 0)
        {
            _listenerMessageFlags[MessageType::GET_SENSOR_DATA_CONFIGURATIONS] = true;
        }
        return dataOffset;
    }
    uint8_t onListenerRequestSetSensorDataConfigurations(uint8_t *data, uint8_t dataOffset)
    {
        auto size = data[dataOffset++];
        sensorData::setConfigurations(&data[dataOffset], size);
        dataOffset += size;
        _listenerMessageFlags.erase(MessageType::GET_SENSOR_DATA_CONFIGURATIONS);
        _listenerMessageFlags[MessageType::SET_SENSOR_DATA_CONFIGURATIONS] = true;
        return dataOffset;
    }

    bool _isParsingPacket = false;
    void onUDPPacket(AsyncUDPPacket packet)
    {
        _isParsingPacket = true;
#if DEBUG
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
        for (uint8_t index = 0; index < packet.length(); index++)
        {
            Serial.print(packetData[index]);
            Serial.print(", ");
        }
        Serial.println();
#endif

        lastTimePacketWasReceivedByListener = currentTime;
        if (!_hasListener)
        {
            _hasListener = true;
            remoteIP = packet.remoteIP();
            remotePort = packet.remotePort();
            _onListenerConnection();
        }
        else
        {
            if (packet.remoteIP() != remoteIP || packet.remotePort() != remotePort)
            {
                Serial.println("not the same IP!");
                return;
            }
        }

        uint8_t dataOffset = 0;
        MessageType messageType;

        auto length = packet.length();
        auto data = packet.data();
        while (dataOffset < length)
        {
            messageType = (MessageType)data[dataOffset++];
#if DEBUG
            Serial.print("[UDP] message type: ");
            Serial.println((uint8_t)messageType);
#endif

            switch (messageType)
            {
            case MessageType::PING:
#if DEBUG
                Serial.println("[udp] received ping from listener");
#endif
                break;
            case MessageType::GET_TYPE:
                dataOffset = onListenerRequestGetType(data, dataOffset);
                break;
            case MessageType::SET_TYPE:
                dataOffset = onListenerRequestSetType(data, dataOffset);
                break;
            case MessageType::GET_NAME:
                dataOffset = onListenerRequestGetName(data, dataOffset);
                break;
            case MessageType::SET_NAME:
                dataOffset = onListenerRequestSetName(data, dataOffset);
                break;
            case MessageType::GET_SENSOR_DATA_CONFIGURATIONS:
                dataOffset = onListenerRequestGetSensorDataConfigurations(data, dataOffset);
                break;
            case MessageType::SET_SENSOR_DATA_CONFIGURATIONS:
                dataOffset = onListenerRequestSetSensorDataConfigurations(data, dataOffset);
                break;
            default:
                Serial.print("uncaught udp message type: ");
                Serial.println((uint8_t)messageType);
                dataOffset = length;
                break;
            }
        }

        shouldSendToListener = shouldSendToListener || (_listenerMessageFlags.size() > 0);
        _isParsingPacket = false;
    }

    const uint16_t listenerTimeout = 2000; // ping every <2 seconds
    void checkListenerConnection()
    {
        if (currentTime - lastTimePacketWasReceivedByListener > listenerTimeout)
        {
            _hasListener = false;
            _onListenerDisconnection();
        }
    }

    void listen(uint16_t port)
    {
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
            _listenerMessageFlags[MessageType::MOTION_CALIBRATION] = true;
            shouldSendToListener = true;
        }
    }
    unsigned long lastSensorDataUpdateTime;
    void sensorDataLoop()
    {
        if (lastSensorDataUpdateTime != sensorData::lastDataUpdateTime && (sensorData::motionDataSize + sensorData::pressureDataSize > 0))
        {
            lastSensorDataUpdateTime = sensorData::lastDataUpdateTime;
            _listenerMessageFlags[MessageType::SENSOR_DATA] = true;
            shouldSendToListener = true;
        }
    }

    uint8_t _listenerMessageData[1 + sizeof(type::Type) + 1 + (sizeof(uint16_t) *  sensorData::configurations.motion.max_size()) + 1 + (sizeof(uint16_t) *  sensorData::configurations.pressure.max_size()) + 1 + sizeof(uint16_t) + 2 + sizeof(sensorData::motionData) + 2 + sizeof(sensorData::pressureData)];
    uint8_t _listenerMessageDataSize = 0;
    void messageLoop()
    {
        if (shouldSendToListener)
        {
            _listenerMessageDataSize = 0;

            for (auto listenerMessageFlagIterator = _listenerMessageFlags.begin(); listenerMessageFlagIterator != _listenerMessageFlags.end(); listenerMessageFlagIterator++)
            {
                auto messageType = listenerMessageFlagIterator->first;
                _listenerMessageData[_listenerMessageDataSize++] = (uint8_t)messageType;

                switch (messageType)
                {
                case MessageType::GET_TYPE:
                case MessageType::SET_TYPE:
                    _listenerMessageData[_listenerMessageDataSize++] = (uint8_t)type::getType();
                    break;
                case MessageType::GET_NAME:
                case MessageType::SET_NAME:
                {
                    auto _name = name::getName();
                    _listenerMessageData[_listenerMessageDataSize++] = _name->length();
                    memcpy(&_listenerMessageData[_listenerMessageDataSize], _name->c_str(), _name->length());
                    _listenerMessageDataSize += _name->length();
                }
                break;
                case MessageType::MOTION_CALIBRATION:
                    memcpy(&_listenerMessageData[_listenerMessageDataSize], motionSensor::calibration, sizeof(motionSensor::calibration));
                    _listenerMessageDataSize += sizeof(motionSensor::calibration);
                    break;
                case MessageType::GET_SENSOR_DATA_CONFIGURATIONS:
                case MessageType::SET_SENSOR_DATA_CONFIGURATIONS:
                    memcpy(&_listenerMessageData[_listenerMessageDataSize], (uint8_t *) sensorData::configurations.motion.data(), sizeof(uint16_t) * sensorData::configurations.motion.max_size());
                    _listenerMessageDataSize += sizeof(uint16_t) * sensorData::configurations.motion.max_size();
                    memcpy(&_listenerMessageData[_listenerMessageDataSize], (uint8_t *) sensorData::configurations.pressure.data(), sizeof(uint16_t) * sensorData::configurations.pressure.max_size());
                    _listenerMessageDataSize += sizeof(uint16_t) * sensorData::configurations.pressure.max_size();
                    break;
                case MessageType::SENSOR_DATA:
                {
                    uint16_t timestamp = (uint16_t)lastSensorDataUpdateTime;
                    MEMCPY(&_listenerMessageData[_listenerMessageDataSize], &timestamp, sizeof(timestamp));
                    _listenerMessageDataSize += sizeof(timestamp);

                    _listenerMessageData[_listenerMessageDataSize++] = (uint8_t)sensorData::SensorType::MOTION;
                    _listenerMessageData[_listenerMessageDataSize++] = sensorData::motionDataSize;
                    memcpy(&_listenerMessageData[_listenerMessageDataSize], sensorData::motionData, sensorData::motionDataSize);
                    _listenerMessageDataSize += sensorData::motionDataSize;

                    _listenerMessageData[_listenerMessageDataSize++] = (uint8_t)sensorData::SensorType::PRESSURE;
                    _listenerMessageData[_listenerMessageDataSize++] = sensorData::pressureDataSize;
                    memcpy(&_listenerMessageData[_listenerMessageDataSize], sensorData::pressureData, sensorData::pressureDataSize);
                    _listenerMessageDataSize += sensorData::pressureDataSize;
                }
                break;
                default:
                    Serial.print("uncaught listener message type: ");
                    Serial.println((uint8_t)messageType);
                    break;
                }
            }

#if DEBUG
            Serial.print("Sending to listener message of size ");
            Serial.print(_listenerMessageDataSize);
            Serial.print(": ");
            for (uint8_t index = 0; index < _listenerMessageDataSize; index++)
            {
                Serial.print(_listenerMessageData[index]);
                Serial.print(',');
            }
            Serial.println();
#endif

            udp.writeTo(_listenerMessageData, _listenerMessageDataSize, remoteIP, remotePort);

            shouldSendToListener = false;
            _listenerMessageFlags.clear();
        }
    }
    void loop()
    {
        currentTime = millis();
        if (_hasListener && !_isParsingPacket)
        {
            checkListenerConnection();
            batteryLevelLoop();
            motionCalibrationLoop();
            sensorDataLoop();
            messageLoop();
        }
    }
} // namespace udp
