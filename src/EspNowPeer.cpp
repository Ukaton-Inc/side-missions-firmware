#include "EspNowPeer.h"

using namespace wifiServer;

std::vector<EspNowPeer *> EspNowPeer::peers;
uint8_t EspNowPeer::getNumberOfPeers()
{
    return peers.size();
};

EspNowPeer::EspNowPeer(const uint8_t *macAddress)
{
    memset(&peerInfo, 0, sizeof(peerInfo));
    peerInfo.encrypt = false;

    setIndex(peers.size());
    peers.push_back(this);

    Serial.print("created a new peer with index: ");
    Serial.println(index);

    setMacAddress(macAddress);
};
EspNowPeer::~EspNowPeer()
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

esp_err_t EspNowPeer::connect()
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
esp_err_t EspNowPeer::disconnect()
{
    return esp_now_del_peer(macAddress);
}
bool EspNowPeer::isConnected()
{
    return esp_now_is_peer_exist(macAddress);
}

void EspNowPeer::onClientConnection()
{
    messageMap[MessageType::CLIENT_CONNECTED];
    messageMap.erase(MessageType::CLIENT_DISCONNECTED);
    shouldSend = true;
}
void EspNowPeer::OnClientConnection()
{
    for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->onClientConnection();
    }
    shouldSendAll = true;
}

void EspNowPeer::onClientDisconnection()
{
    messageMap[MessageType::CLIENT_DISCONNECTED];
    messageMap.erase(MessageType::CLIENT_CONNECTED);
    shouldSend = true;
}
void EspNowPeer::OnClientDisconnection()
{
    for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->onClientDisconnection();
    }
    shouldSendAll = true;
}

uint8_t EspNowPeer::getIndex()
{
    return index;
}
void EspNowPeer::setIndex(uint8_t _index)
{
    index = _index;
    deviceIndex = index + 1;
}

uint8_t EspNowPeer::getDeviceIndex()
{
    return deviceIndex;
}

const uint8_t *EspNowPeer::getMacAddress()
{
    return macAddress;
}
void EspNowPeer::setMacAddress(const uint8_t *_macAddress)
{
    memcpy(macAddress, _macAddress, sizeof(macAddress));
    memcpy(peerInfo.peer_addr, macAddress, sizeof(macAddress));

    connect();
}
EspNowPeer *EspNowPeer::getPeerByMacAddress(const uint8_t *macAddress)
{
    EspNowPeer *peer = nullptr;
    for (auto peerIterator = peers.begin(); peerIterator != peers.end() && peer == nullptr; peerIterator++)
    {
        if (areMacAddressesEqual((*peerIterator)->macAddress, macAddress))
        {
            peer = (*peerIterator);
        }
    }
    return peer;
}
EspNowPeer *EspNowPeer::getPeerByDeviceIndex(uint8_t deviceIndex)
{
    try
    {
        EspNowPeer *peer = peers.at(deviceIndex - 1);
        return peer;
    }
    catch (const std::out_of_range &error)
    {
        Serial.print("out of range error: ");
        Serial.println(error.what());
        throw error;
    }
}

bool EspNowPeer::getAvailability()
{
    return isAvailable;
}
void EspNowPeer::updateAvailability(bool _isAvailable)
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
        didUpdateNameAtLeastOnce = false;
        didUpdateDeviceTypeAtLeastOnce = false;
    }

    Serial.print("updated availability: ");
    Serial.println(isAvailable);
}

uint8_t EspNowPeer::getBatteryLevel()
{
    return batteryLevel;
}
void EspNowPeer::updateBatteryLevel(uint8_t _batteryLevel)
{
    batteryLevel = _batteryLevel;
    didSendBatteryLevel = false;
    didUpdateBatteryLevelAtLeastOnce = true;

    Serial.print("updated battery level: ");
    Serial.println(batteryLevel);
}

const std::string *EspNowPeer::getName()
{
    return &name;
}

void EspNowPeer::updateName(const char *_name, size_t length)
{
    name.assign(_name, length);
    didUpdateNameAtLeastOnce = true;
    Serial.print("updated name to: ");
    Serial.println(name.c_str());
}

DeviceType EspNowPeer::getDeviceType()
{
    return deviceType;
}
void EspNowPeer::updateDeviceType(DeviceType _deviceType)
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

EspNowPeer::Motion::~Motion()
{
    delete[] data;
    delete dataTypes;
}
void EspNowPeer::Motion::updateCalibration(const uint8_t *_calibration)
{
    memcpy(calibration, _calibration, sizeof(calibration));
    didUpdateCalibrationAtLeastOnce = true;
    didSendCalibration = false;

    Serial.print("updated calibration: ");
    for (uint8_t index = 0; index < sizeof(calibration); index++)
    {
        Serial.print(calibration[index]);
        Serial.print(',');
    }
    Serial.println();
}
uint8_t EspNowPeer::Motion::onCalibration(const uint8_t *_calibration, uint8_t incomingDataOffset)
{
    updateCalibration(_calibration);
    incomingDataOffset += sizeof(calibration);
    return incomingDataOffset;
}

EspNowPeer::Pressure::~Pressure()
{
    delete[] data;
    delete dataTypes;
}

void EspNowPeer::send()
{
    if (shouldSend)
    {
        std::vector<uint8_t> data;
        for (auto messageMapIterator = messageMap.begin(); messageMapIterator != messageMap.end(); messageMapIterator++)
        {
            data.push_back((uint8_t)messageMapIterator->first);
            data.insert(data.end(), messageMapIterator->second.begin(), messageMapIterator->second.end());
        }

        Serial.print("Sending to Peer #");
        Serial.print(index);
        Serial.print(": ");
        for (uint8_t index = 0; index < data.size(); index++)
        {
            Serial.print(data[index]);
            Serial.print(',');
        }
        Serial.println();

        esp_err_t sendError = esp_now_send(macAddress, data.data(), data.size());

        if (sendError == ESP_OK)
        {
            Serial.println("Delivery Success");
        }
        else
        {
            Serial.print("Delivery Failed: ");
            Serial.println(esp_err_to_name(sendError));
        }

        shouldSend = false;
    }
}
bool EspNowPeer::shouldSendAll = false;
void EspNowPeer::sendAll()
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

void EspNowPeer::ping()
{
    if (messageMap.size() == 0)
    {
        messageMap[MessageType::PING];
        shouldSend = true;
    }
}
void EspNowPeer::pingAll()
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
unsigned long EspNowPeer::previousPingMillis = 0;
void EspNowPeer::pingLoop()
{
    if (currentMillis - previousPingMillis >= pingInterval)
    {
        previousPingMillis = currentMillis;
        pingAll();
    }
}

uint8_t EspNowPeer::onBatteryLevel(const uint8_t *incomingData, uint8_t incomingDataOffset)
{
    uint8_t batteryLevel = incomingData[incomingDataOffset++];
    updateBatteryLevel(batteryLevel);
    return incomingDataOffset;
}
uint8_t EspNowPeer::onName(const uint8_t *incomingData, uint8_t incomingDataOffset, MessageType messageType)
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
uint8_t EspNowPeer::onType(const uint8_t *incomingData, uint8_t incomingDataOffset)
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
void EspNowPeer::onMessage(const uint8_t *incomingData, int len)
{
    Serial.print("EspNowPeer Message: ");
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
        Serial.print("Message Type from peer: ");
        Serial.println((uint8_t)messageType);
        switch (messageType)
        {
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
        {
            auto motionCalibration = &incomingData[incomingDataOffset++];
            incomingDataOffset = motion.onCalibration(motionCalibration, incomingDataOffset);
        }
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

void EspNowPeer::batteryLevelLoop()
{
    if (isAvailable && !didSendBatteryLevel && didUpdateBatteryLevelAtLeastOnce)
    {
        uint8_t data[1]{batteryLevel};
        deviceClientMessageMaps[deviceIndex][MessageType::BATTERY_LEVEL].assign(data, data + sizeof(data));
        didSendBatteryLevel = true;
        shouldSendToClient = true;
    }
}
void EspNowPeer::batteryLevelsLoop()
{
    for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->batteryLevelLoop();
    }
}

void EspNowPeer::motionCalibrationLoop()
{
    if (isAvailable && !motion.didSendCalibration && motion.didUpdateCalibrationAtLeastOnce)
    {
        deviceClientMessageMaps[deviceIndex][MessageType::MOTION_CALIBRATION].assign(motion.calibration, motion.calibration + sizeof(motion.calibration));
        motion.didSendCalibration = true;
        shouldSendToClient = true;
    }
}
void EspNowPeer::motionCalibrationsLoop()
{
    for (auto peerIterator = peers.begin(); peerIterator != peers.end(); peerIterator++)
    {
        (*peerIterator)->motionCalibrationLoop();
    }
}