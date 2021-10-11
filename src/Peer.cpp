#include "Peer.h"
#include "definitions.h"

using namespace wifiServer;

std::vector<Peer *> Peer::peers;
uint8_t Peer::getNumberOfPeers()
{
    return peers.size();
};

Peer::Peer(const uint8_t *macAddress)
{
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.encrypt = false;

    setIndex(peers.size());
    peers.push_back(this);

    Serial.print("created a new peer with index: ");
    Serial.println(index);

    setMacAddress(macAddress);
};
Peer::~Peer()
{
    disconnect();
    delete pressure;
    messageMap.clear();
    deviceClientMessageMaps.erase(deviceIndex);
    for (auto peerIterator = peers.erase(peers.begin() + index); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->setIndex((*peerIterator)->getIndex() - 1);
    }
}

esp_err_t Peer::connect()
{
    esp_err_t result = esp_now_add_peer(&peerInfo);
    if (result == ESP_OK)
    {
        Serial.print("Successfully added peer #");
        Serial.print(index);
        Serial.print(", device #");
        Serial.println(deviceIndex);
        if (isConnectedToClient())
        {
            deviceClientMessageMaps[deviceIndex][MessageType::DEVICE_ADDED];
        }
    }
    else
    {
        Serial.println("Failed to add peer");
        Serial.println(esp_err_to_name(result));
    }
    return result;
}
esp_err_t Peer::disconnect()
{
    return esp_now_del_peer(macAddress);
}
bool Peer::isConnected()
{
    return esp_now_is_peer_exist(macAddress);
}

void Peer::onClientConnection()
{
    messageMap[MessageType::CLIENT_CONNECTED];
    messageMap.erase(MessageType::CLIENT_DISCONNECTED);
    shouldSend = true;
}
void Peer::OnClientConnection()
{
    for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->onClientConnection();
    }
    shouldSendAll = true;
}

void Peer::onClientDisconnection()
{
    messageMap[MessageType::CLIENT_DISCONNECTED];
    messageMap.erase(MessageType::CLIENT_CONNECTED);
    shouldSend = true;
    memset(&motion.configuration, 0, sizeof(motion.configuration));
}
void Peer::OnClientDisconnection()
{
    for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->onClientDisconnection();
    }
    shouldSendAll = true;
}

uint8_t Peer::getIndex()
{
    return index;
}
void Peer::setIndex(uint8_t _index)
{
    index = _index;
    deviceIndex = index + 1;
}

uint8_t Peer::getDeviceIndex()
{
    return deviceIndex;
}

const uint8_t *Peer::getMacAddress()
{
    return macAddress;
}
void Peer::setMacAddress(const uint8_t *_macAddress)
{
    memcpy(macAddress, _macAddress, sizeof(macAddress));
    memcpy(peerInfo.peer_addr, macAddress, sizeof(macAddress));

    connect();
}
Peer *Peer::getPeerByMacAddress(const uint8_t *macAddress)
{
    Peer *peer = nullptr;
    for (auto peerIterator = peers.begin(); peerIterator != peers.end() && peer == nullptr; peerIterator++)
    {
        if (areMacAddressesEqual((*peerIterator)->macAddress, macAddress))
        {
            peer = (*peerIterator);
        }
    }
    return peer;
}
Peer *Peer::getPeerByDeviceIndex(uint8_t deviceIndex)
{
    try
    {
        Peer *peer = peers.at(deviceIndex - 1);
        return peer;
    }
    catch (const std::out_of_range &error)
    {
        Serial.print("out of range error: ");
        Serial.println(error.what());
        throw error;
    }
}

bool Peer::getAvailability()
{
    return isAvailable;
}
void Peer::updateAvailability(bool _isAvailable)
{
    if (isAvailable != _isAvailable)
    {
        uint8_t data[1];
        data[0] = _isAvailable ? 1 : 0;
        deviceClientMessageMaps[deviceIndex][MessageType::AVAILABILITY].assign(data, data + sizeof(data));
    }

    isAvailable = _isAvailable;

    if (!isAvailable)
    {
        didUpdateDelayAtLeastOnce = false;
        didUpdateNameAtLeastOnce = false;
        didUpdateDeviceTypeAtLeastOnce = false;
        didUpdateBatteryLevelAtLeastOnce = false;
        motion.didUpdateCalibrationAtLeastOnce = false;
        motion.didUpdateConfigurationAtLeastOnce = false;
    }

#if DEBUG
    Serial.print("updated availability: ");
    Serial.println(isAvailable);
#endif
}

void Peer::updateDelay() {
    if (!didUpdateDelayAtLeastOnce) {
        delayMillis = millis() % dataInterval;
        messageMap[MessageType::TIMESTAMP_DELAY].assign((uint8_t *)&delayMillis, ((uint8_t *)&delayMillis) + sizeof(delayMillis));

        #if DEBUG
        Serial.print("updated delay: ");
        Serial.println(delayMillis);
        #endif

        didUpdateDelayAtLeastOnce = true;
    }
}

unsigned long Peer::getTimestamp()
{
    return timestamp;
}
void Peer::updateTimestamp(unsigned long _timestamp)
{
    timestamp = _timestamp;
    didUpdateTimestampAtLeastOnce = true;

#if DEBUG
    Serial.print("updated timestamp: ");
    Serial.println(timestamp);
#endif
}

uint8_t Peer::getBatteryLevel()
{
    return batteryLevel;
}
void Peer::updateBatteryLevel(uint8_t _batteryLevel)
{
    batteryLevel = _batteryLevel;
    didSendBatteryLevel = false;
    didUpdateBatteryLevelAtLeastOnce = true;

#if DEBUG
    Serial.print("updated battery level: ");
    Serial.println(batteryLevel);
#endif
}

const std::string *Peer::getName()
{
    return &name;
}

void Peer::updateName(const char *_name, size_t length)
{
    name.assign(_name, length);
    didUpdateNameAtLeastOnce = true;

    Serial.print("updated name to: ");
    Serial.println(name.c_str());
}

DeviceType Peer::getDeviceType()
{
    return deviceType;
}
void Peer::updateDeviceType(DeviceType _deviceType)
{
    deviceType = _deviceType;
    didUpdateDeviceTypeAtLeastOnce = true;
    Serial.print("updating device type: ");
    Serial.println((uint8_t)deviceType);

    delete pressure;
    if (deviceType == DeviceType::LEFT_INSOLE || deviceType == DeviceType::RIGHT_INSOLE)
    {
        pressure = new Pressure();
    }
}

void Peer::Motion::updateCalibration(const uint8_t *_calibration)
{
    memcpy(calibration, _calibration, sizeof(calibration));
    didUpdateCalibrationAtLeastOnce = true;
    didSendCalibration = false;

#if DEBUG
    Serial.print("updated calibration: ");
    for (uint8_t index = 0; index < sizeof(calibration); index++)
    {
        Serial.print(calibration[index]);
        Serial.print(',');
    }
    Serial.println();
#endif
}
uint8_t Peer::onMotionCalibration(const uint8_t *incomingData, uint8_t incomingDataOffset)
{
    auto motionCalibration = &incomingData[incomingDataOffset];
    motion.updateCalibration(motionCalibration);
    incomingDataOffset += sizeof(motion.calibration);
    return incomingDataOffset;
}

void Peer::Motion::updateConfiguration(const uint16_t *_configuration)
{
    memcpy(configuration, _configuration, sizeof(configuration));
    for (uint8_t dataType = 0; dataType < (uint8_t)motion::DataType::COUNT; dataType++)
    {
        configuration[dataType] -= configuration[dataType] % dataInterval;
    }
    didUpdateConfigurationAtLeastOnce = true;

#if DEBUG
    Serial.print("updated configuration: ");
    for (uint8_t index = 0; index < (sizeof(configuration) / 2); index++)
    {
        Serial.print(configuration[index]);
        Serial.print(',');
    }
    Serial.println();
#endif
}
uint8_t Peer::onMotionConfiguration(const uint8_t *incomingData, uint8_t incomingDataOffset, MessageType messageType)
{
    ErrorMessageType errorMessageType = (ErrorMessageType)incomingData[incomingDataOffset++];
    if (errorMessageType == ErrorMessageType::NO_ERROR)
    {
        auto motionConfiguration = (const uint16_t *)&incomingData[incomingDataOffset];
        motion.updateConfiguration(motionConfiguration);
        incomingDataOffset += sizeof(motion.configuration);

        if (isConnectedToClient())
        {
            std::vector<uint8_t> data;
            data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
            data.insert(data.end(), (uint8_t *)motion.configuration, ((uint8_t *)motion.configuration) + sizeof(motion.configuration));
            deviceClientMessageMaps[deviceIndex][messageType] = data;
        }
    }
    else
    {
        if (isConnectedToClient())
        {
            uint8_t data[1]{(uint8_t)errorMessageType};
            deviceClientMessageMaps[deviceIndex][messageType].assign(data, data + sizeof(data));
        }
    }

    return incomingDataOffset;
}

uint8_t Peer::onMotionData(const uint8_t *incomingData, uint8_t incomingDataOffset)
{
    uint8_t motionDataLength = incomingData[incomingDataOffset++];
    motion.updateData(&incomingData[incomingDataOffset], motionDataLength);
    incomingDataOffset += motionDataLength;
    return incomingDataOffset;
}
void Peer::Motion::updateData(const uint8_t *_data, size_t length)
{
    uint8_t dataOffset = 0;
    while (dataOffset < length)
    {
        auto dataType = (motion::DataType)_data[dataOffset++];
        uint8_t dataSize = 0;
        switch (dataType)
        {
        case motion::DataType::ACCELERATION:
        case motion::DataType::GRAVITY:
        case motion::DataType::LINEAR_ACCELERATION:
        case motion::DataType::ROTATION_RATE:
        case motion::DataType::MAGNETOMETER:
            dataSize = 6;
            break;
        case motion::DataType::QUATERNION:
            dataSize = 8;
            break;
        default:
            Serial.print("uncaught motion data type: ");
            Serial.println((uint8_t)dataType);
            break;
        }

        if (dataSize > 0)
        {
            data[dataType].assign((int16_t *)&_data[dataOffset], (int16_t *)(&_data[dataOffset] + dataSize));
            didSendData[dataType] = false;
            dataOffset += dataSize;

#if DEBUG
            Serial.print("Motion Data type #");
            Serial.print((uint8_t)dataType);
            Serial.print(": ");
            for (auto iterator = data[dataType].begin(); iterator != data[dataType].end(); iterator++)
            {
                Serial.print(*iterator);
                Serial.print(',');
            }
            Serial.println();
#endif
        }
    }
}

std::vector<uint8_t> Peer::Motion::getData()
{
    std::vector<uint8_t> motionData;
    for (uint8_t dataTypeIndex = 0; dataTypeIndex < (uint8_t)motion::DataType::COUNT; dataTypeIndex++)
    {
        auto dataType = (motion::DataType)dataTypeIndex;

        if (configuration[dataTypeIndex] != 0 && previousDataMillis % configuration[dataTypeIndex] == 0)
        {
            if (didSendData.count(dataType) == 1 && !didSendData[dataType] && data.count(dataType) == 1) {
                motionData.push_back((uint8_t)dataType);
                for (auto iterator = data[dataType].begin(); iterator != data[dataType].end(); iterator++) {
                    motionData.push_back(lowByte(*iterator));
                    motionData.push_back(highByte(*iterator));
                }
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

void Peer::send()
{
    if (shouldSend)
    {
        std::vector<uint8_t> data;
        for (auto messageMapIterator = messageMap.begin(); messageMapIterator != messageMap.end(); messageMapIterator++)
        {
            data.push_back((uint8_t)messageMapIterator->first);
            data.insert(data.end(), messageMapIterator->second.begin(), messageMapIterator->second.end());
        }

#if DEBUG
        Serial.print("Sending to Peer #");
        Serial.print(index);
        Serial.print(": ");
        for (uint8_t index = 0; index < data.size(); index++)
        {
            Serial.print(data[index]);
            Serial.print(',');
        }
        Serial.println();
#endif

        esp_err_t sendError = esp_now_send(macAddress, data.data(), data.size());

        if (sendError == ESP_OK)
        {
#if DEBUG
            Serial.println("Delivery Success");
#endif
        }
        else
        {
#if DEBUG
            Serial.print("Delivery Failed: ");
            Serial.println(esp_err_to_name(sendError));
#endif
        }

        shouldSend = false;
    }
}
bool Peer::shouldSendAll = false;
void Peer::sendAll()
{
    if (shouldSendAll)
    {
        for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
        {
            (*peerIterator)->send();
        }
        shouldSendAll = false;
    }
}

void Peer::ping()
{
    if (messageMap.size() == 0)
    {
        messageMap[MessageType::PING];
        shouldSend = true;
    }
}
void Peer::pingAll()
{
    if (peers.size() > 0)
    {
        for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
        {
            (*peerIterator)->ping();
        }
        shouldSendAll = true;
    }
}
unsigned long Peer::previousPingMillis = 0;
void Peer::pingLoop()
{
    if (currentMillis - previousPingMillis >= pingInterval)
    {
        pingAll();
        previousPingMillis = currentMillis - (currentMillis % pingInterval);
    }
}

uint8_t Peer::onTimestamp(const uint8_t *incomingData, uint8_t incomingDataOffset)
{
    unsigned long timestamp;
    memcpy(&timestamp, &incomingData[incomingDataOffset], sizeof(timestamp));
    incomingDataOffset += sizeof(timestamp);
    updateTimestamp(timestamp);
    return incomingDataOffset;
}
uint8_t Peer::onBatteryLevel(const uint8_t *incomingData, uint8_t incomingDataOffset)
{
    uint8_t batteryLevel = incomingData[incomingDataOffset++];
    updateBatteryLevel(batteryLevel);
    return incomingDataOffset;
}
uint8_t Peer::onName(const uint8_t *incomingData, uint8_t incomingDataOffset, MessageType messageType)
{
    ErrorMessageType errorMessageType = (ErrorMessageType)incomingData[incomingDataOffset++];
    if (errorMessageType == ErrorMessageType::NO_ERROR)
    {
        uint8_t peerNameLength = incomingData[incomingDataOffset++];
        auto peerName = (const char *)&incomingData[incomingDataOffset];
        incomingDataOffset += peerNameLength;
        updateName(peerName, peerNameLength);
        if (isConnectedToClient())
        {
            std::vector<uint8_t> data;
            data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
            data.push_back(name.length());
            data.insert(data.end(), name.begin(), name.end());
            deviceClientMessageMaps[deviceIndex][messageType] = data;
        }
    }
    else
    {
        if (isConnectedToClient())
        {
            uint8_t data[1]{(uint8_t)errorMessageType};
            deviceClientMessageMaps[deviceIndex][messageType].assign(data, data + sizeof(data));
        }
    }
    return incomingDataOffset;
}
uint8_t Peer::onType(const uint8_t *incomingData, uint8_t incomingDataOffset)
{
    ErrorMessageType errorMessageType = (ErrorMessageType)incomingData[incomingDataOffset++];
    if (errorMessageType == ErrorMessageType::NO_ERROR)
    {
        DeviceType peerDeviceType = (DeviceType)incomingData[incomingDataOffset++];
        updateDeviceType(peerDeviceType);

        if (isConnectedToClient())
        {
            std::vector<uint8_t> data;
            data.push_back((uint8_t)ErrorMessageType::NO_ERROR);
            data.push_back((uint8_t)deviceType);

            deviceClientMessageMaps[deviceIndex][MessageType::GET_TYPE] = data;
        }
    }
    else
    {
        if (isConnectedToClient())
        {
            uint8_t data[1]{(uint8_t)errorMessageType};
            deviceClientMessageMaps[deviceIndex][MessageType::GET_TYPE].assign(data, data + sizeof(data));
        }
    }
    return incomingDataOffset;
}
void Peer::onMessage(const uint8_t *incomingData, int len)
{
#if DEBUG
    Serial.print("Peer Message: ");
    for (uint8_t index = 0; index < len; index++)
    {
        Serial.print(incomingData[index]);
        Serial.print(',');
    }
    Serial.println();
#endif

    updateDelay();

    uint8_t incomingDataOffset = 0;
    MessageType messageType;
    while (incomingDataOffset < len)
    {
        messageType = (MessageType)incomingData[incomingDataOffset++];
#if DEBUG
        Serial.print("Message Type from peer: ");
        Serial.println((uint8_t)messageType);
#endif
        switch (messageType)
        {
        case MessageType::TIMESTAMP:
            incomingDataOffset = onTimestamp(incomingData, incomingDataOffset);
            break;
        case MessageType::BATTERY_LEVEL:
            incomingDataOffset = onBatteryLevel(incomingData, incomingDataOffset);
            break;
        case MessageType::GET_NAME:
        case MessageType::SET_NAME:
            incomingDataOffset = onName(incomingData, incomingDataOffset, messageType);
            break;
        case MessageType::GET_TYPE:
            incomingDataOffset = onType(incomingData, incomingDataOffset);
            break;
        case MessageType::MOTION_CALIBRATION:
            incomingDataOffset = onMotionCalibration(incomingData, incomingDataOffset);
            break;
        case MessageType::GET_MOTION_CONFIGURATION:
        case MessageType::SET_MOTION_CONFIGURATION:
            incomingDataOffset = onMotionConfiguration(incomingData, incomingDataOffset, messageType);
            break;
        case MessageType::MOTION_DATA:
            incomingDataOffset = onMotionData(incomingData, incomingDataOffset);
            break;
        case MessageType::PRESSURE_DATA:
            //incomingDataOffset = onPressureData(incomingData, incomingDataOffset);
            break;
        case MessageType::PING:
            break;
        default:
            Serial.print("uncaught peer message type: ");
            Serial.println((uint8_t)messageType);
            incomingDataOffset = len;
            break;
        }
    }

    shouldSend = (messageMap.size() > 0);
    shouldSendAll = shouldSendAll || shouldSend;

    shouldSendToClient = shouldSendToClient || (clientMessageMap.size() > 0) || (deviceClientMessageMaps.size() > 0);
}

void Peer::batteryLevelLoop()
{
    if (isAvailable && !didSendBatteryLevel && didUpdateBatteryLevelAtLeastOnce)
    {
        uint8_t data[1]{batteryLevel};
        deviceClientMessageMaps[deviceIndex][MessageType::BATTERY_LEVEL].assign(data, data + sizeof(data));
        didSendBatteryLevel = true;
        shouldSendToClient = true;
    }
}
void Peer::BatteryLevelLoop()
{
    for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->batteryLevelLoop();
    }
}

void Peer::motionCalibrationLoop()
{
    if (isAvailable && !motion.didSendCalibration && motion.didUpdateCalibrationAtLeastOnce)
    {
        deviceClientMessageMaps[deviceIndex][MessageType::MOTION_CALIBRATION].assign(motion.calibration, motion.calibration + sizeof(motion.calibration));
        motion.didSendCalibration = true;
        shouldSendToClient = true;
    }
}
void Peer::MotionCalibrationLoop()
{
    for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->motionCalibrationLoop();
    }
}

void Peer::motionDataLoop()
{
    if (isAvailable)
    {
        auto motionData = motion.getData();
        if (motionData.size() > 0) {
            deviceClientMessageMaps[deviceIndex][MessageType::MOTION_DATA].push_back(motionData.size());
            deviceClientMessageMaps[deviceIndex][MessageType::MOTION_DATA].insert(deviceClientMessageMaps[deviceIndex][MessageType::MOTION_DATA].end(), motionData.begin(), motionData.end());

            shouldSendToClient = true;
            includeTimestampInClientMessage = true;

            motion.didSendData.clear();
        }
    }
}
void Peer::MotionDataLoop()
{
    for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->motionDataLoop();
    }
}