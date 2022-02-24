#include "BLEPeer.h"
#include "information/name.h"
#include "information/bleName.h"
#include "information/bleType.h"
#include "sensor/bleSensorData.h"

BLEPeer BLEPeer::peers[NIMBLE_MAX_CONNECTIONS];

BLEScan *BLEPeer::pBLEScan;
void BLEPeer::AdvertisedDeviceCallbacks::onResult(NimBLEAdvertisedDevice *advertisedDevice)
{
    if (advertisedDevice->getServiceUUID() == ble::pService->getUUID())
    {
        Serial.printf("found device \"%s\"\n", advertisedDevice->getName().c_str());

        for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
        {
            if (peers[index].autoConnect && (peers[index].pClient == nullptr || !peers[index].pClient->isConnected()) && peers[index]._name == advertisedDevice->getName())
            {
                peers[index].pAdvertisedDevice = advertisedDevice;
                peers[index].foundDevice = true;
                break;
            }
        }
    }
}

void BLEPeer::setConfigurations(const uint8_t *newConfigurations, uint8_t size)
{
    uint8_t offset = 0;
    while (offset < size)
    {
        const auto sensorType = (sensorData::SensorType)newConfigurations[offset++];
        if (sensorData::isValidSensorType(sensorType))
        {
            const uint8_t _size = newConfigurations[offset++];
            setConfiguration(&newConfigurations[offset], _size, sensorType);
            offset += _size;
        }
        else {
            Serial.printf("invalid sensor type: %d\n", (uint8_t) sensorType);
            break;
        }
    }

    uint8_t _offset = 0;
    memcpy(&sensorConfiguration[_offset], motionConfiguration, sizeof(motionConfiguration));
    _offset += sizeof(motionConfiguration);
    memcpy(&sensorConfiguration[_offset], pressureConfiguration, sizeof(pressureConfiguration));
    _offset += sizeof(pressureConfiguration);

    pSensorConfigurationCharacteristic->setValue((uint8_t *) sensorConfiguration, sizeof(sensorConfiguration));
}
void BLEPeer::setConfiguration(const uint8_t *newConfiguration, uint8_t size, sensorData::SensorType sensorType)
{
    for (uint8_t offset = 0; offset < size; offset += 3)
    {
        auto sensorDataTypeIndex = newConfiguration[offset];
        uint16_t delay = ((uint16_t)newConfiguration[offset + 2] << 8) | (uint16_t)newConfiguration[offset + 1];
        delay -= (delay % sensorData::min_delay_ms);

        switch (sensorType)
        {
        case sensorData::SensorType::MOTION:
            if (motionSensor::isValidDataType((motionSensor::DataType)sensorDataTypeIndex))
            {
                motionConfiguration[sensorDataTypeIndex] = delay;
            }
            break;
        case sensorData::SensorType::PRESSURE:
            if (pressureSensor::isValidDataType((pressureSensor::DataType)sensorDataTypeIndex))
            {
                pressureConfiguration[sensorDataTypeIndex] = delay;
            }
            break;
        default:
            break;
        }
    }

    if (sensorType == sensorData::SensorType::PRESSURE)
    {
        if (pressureConfiguration[(uint8_t)pressureSensor::DataType::SINGLE_BYTE] > 0 && pressureConfiguration[(uint8_t)pressureSensor::DataType::DOUBLE_BYTE] > 0)
        {
            pressureConfiguration[(uint8_t)pressureSensor::DataType::SINGLE_BYTE] = 0;
        }
    }
}
void BLEPeer::clearConfigurations()
{
    for (uint8_t index = 0; index < (uint8_t)sensorData::SensorType::COUNT; index++)
    {
        auto sensorType = (sensorData::SensorType)index;
        clearConfiguration(sensorType);
    }
}
void BLEPeer::clearConfiguration(sensorData::SensorType sensorType)
{
    switch (sensorType)
    {
    case sensorData::SensorType::MOTION:
        memset(motionConfiguration, 0, sizeof(motionConfiguration));
        break;
    case sensorData::SensorType::PRESSURE:
        memset(pressureConfiguration, 0, sizeof(pressureConfiguration));
        break;
    default:
        break;
    }
}

void BLEPeer::formatBLECharacteristicUUID(char *uuidBuffer, uint8_t value)
{
    snprintf(uuidBuffer, BLE_UUID_LENGTH + 1, "%s%s%d%d%s", UUID_PREFIX, BLE_PEER_UUID_PREFIX, index, value, UUID_SUFFIX);
}
void BLEPeer::formatBLECharacteristicName(char *nameBuffer, const char *_name)
{
    snprintf(nameBuffer, MAX_BLE_ATTRIBUTE_LENGTH + 1, "peer #%d %s", index, _name);
}

void BLEPeer::setup()
{
    for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        peers[index]._setup(index);
    }

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setAdvertisedDeviceCallbacks(new BLEPeer::AdvertisedDeviceCallbacks(), false);

    updateShouldScan();
}
void BLEPeer::_setup(uint8_t _index)
{
    index = _index;

    char preferencesKey[BLE_UUID_LENGTH + 1];
    snprintf(preferencesKey, sizeof(preferencesKey), "blePeer%d", index);
    preferences.begin(preferencesKey);

    char uuidBuffer[BLE_UUID_LENGTH + 1];
    char nameBuffer[MAX_BLE_ATTRIBUTE_LENGTH + 1];

    formatBLECharacteristicUUID(uuidBuffer, 0);
    formatBLECharacteristicName(nameBuffer, "name");
    pNameCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    if (preferences.isKey("name"))
    {
        _name = preferences.getString("name").c_str();
    }
    pNameCharacteristic->setValue(_name);
    pNameCharacteristic->setCallbacks(this);

    formatBLECharacteristicUUID(uuidBuffer, 1);
    formatBLECharacteristicName(nameBuffer, "connect");
    pConnectCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    if (preferences.isKey("connect"))
    {
        autoConnect = preferences.getBool("connect");
    }
    pConnectCharacteristic->setValue((uint8_t *)&autoConnect, 1);
    pConnectCharacteristic->setCallbacks(this);

    formatBLECharacteristicUUID(uuidBuffer, 2);
    formatBLECharacteristicName(nameBuffer, "isConnected");
    pIsConnectedCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, nameBuffer);
    updateIsConnectedCharacteristic(false);

    formatBLECharacteristicUUID(uuidBuffer, 3);
    formatBLECharacteristicName(nameBuffer, "type");
    pTypeCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    pTypeCharacteristic->setValue((uint8_t *)&_type, 1);
    pTypeCharacteristic->setCallbacks(this);

    formatBLECharacteristicUUID(uuidBuffer, 4);
    formatBLECharacteristicName(nameBuffer, "sensor configuration");
    pSensorConfigurationCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    pSensorConfigurationCharacteristic->setValue(sensorConfiguration, sizeof(sensorConfiguration));
    pSensorConfigurationCharacteristic->setCallbacks(this);

    formatBLECharacteristicUUID(uuidBuffer, 5);
    formatBLECharacteristicName(nameBuffer, "sensor data");
    pSensorDataCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::NOTIFY, nameBuffer);
}
BLEPeer::~BLEPeer()
{
    if (pNameCharacteristic != nullptr)
    {
        ble::pService->removeCharacteristic(pNameCharacteristic, true);
        delete pNameCharacteristic;
    }
    if (pConnectCharacteristic != nullptr)
    {
        ble::pService->removeCharacteristic(pConnectCharacteristic, true);
        delete pConnectCharacteristic;
    }
    if (pIsConnectedCharacteristic != nullptr)
    {
        ble::pService->removeCharacteristic(pIsConnectedCharacteristic, true);
        delete pIsConnectedCharacteristic;
    }
    if (pTypeCharacteristic != nullptr)
    {
        ble::pService->removeCharacteristic(pTypeCharacteristic, true);
        delete pTypeCharacteristic;
    }
    if (pSensorConfigurationCharacteristic != nullptr)
    {
        ble::pService->removeCharacteristic(pSensorConfigurationCharacteristic, true);
        delete pSensorConfigurationCharacteristic;
    }
    if (pSensorDataCharacteristic != nullptr)
    {
        ble::pService->removeCharacteristic(pSensorDataCharacteristic, true);
        delete pSensorDataCharacteristic;
    }
}

void BLEPeer::onNameWrite()
{
    auto __name = pNameCharacteristic->getValue();
    if (__name.length() <= name::MAX_NAME_LENGTH)
    {
        _name = __name;
        preferences.putString("name", _name.c_str());

        Serial.print("updated name to: ");
        Serial.println(_name.c_str());

        if (isConnected()) {
            shouldChangeName = true;
        }
    }
    else
    {
        log_e("name's too long");
    }

    pNameCharacteristic->setValue(_name);
}
void BLEPeer::changeName() {
    Serial.printf("setting remote name to %s\n", _name.c_str());
    pRemoteNameCharacteristic->writeValue(_name, true);

    _name = pRemoteNameCharacteristic->readValue();
    Serial.printf("updated remote name: %s\n", _name.c_str());

    pNameCharacteristic->setValue(_name);
}

void BLEPeer::onConnectWrite()
{
    auto _autoConnect = (bool)pConnectCharacteristic->getValue().data()[0];
    if (_autoConnect != autoConnect)
    {
        autoConnect = _autoConnect;
        preferences.putBool("connect", autoConnect);

        Serial.print("updated autoConnect to ");
        Serial.println(autoConnect);

        if (!autoConnect && pClient != nullptr)
        {
            Serial.println("will disconnect client");
            shouldDisconnect = true;
        }

        updateShouldScan();
    }
}

void BLEPeer::onTypeWrite()
{
    if (isConnected()) {
        typeToChangeTo = (type::Type) pTypeCharacteristic->getValue().data()[0];
        shouldChangeType = true;
    }
}
void BLEPeer::changeType() {
    Serial.printf("writing remote type %d\n", (uint8_t) typeToChangeTo);
    pRemoteTypeCharacteristic->writeValue((uint8_t)typeToChangeTo, true);
    Serial.println("wrote remote type!");
    _type = (type::Type) pRemoteTypeCharacteristic->readValue().data()[0];
    Serial.println((uint8_t)_type);
    Serial.printf("read remote type %d\n", (uint8_t) _type);
    pTypeCharacteristic->setValue((uint8_t) _type);
    Serial.println("set value!");
}

void BLEPeer::onSensorConfigurationWrite()
{
    if (isConnected()) {
        auto newConfigurations = pSensorConfigurationCharacteristic->getValue();
        setConfigurations((uint8_t *) newConfigurations.data(), newConfigurations.length());
        receivedConfiguration = newConfigurations;
        shouldChangeSensorConfiguration = true;
    }
}
void BLEPeer::changeSensorConfiguration() {
    //pRemoteSensorConfigurationCharacteristic->writeValue(sensorConfiguration, sizeof(sensorConfiguration), true);
    pRemoteSensorConfigurationCharacteristic->writeValue(receivedConfiguration, true);

    /*
    auto _sensorConfiguration = pRemoteSensorConfigurationCharacteristic->readValue().data();
    memcpy(sensorConfiguration, _sensorConfiguration, sizeof(sensorConfiguration));
    pSensorConfigurationCharacteristic->setValue((uint8_t *) sensorConfiguration, sizeof(sensorConfiguration));
    */
}

void BLEPeer::onWrite(BLECharacteristic *pCharacteristic)
{
    Serial.print("[peer #");
    Serial.print("] wrote to characteristic ");
    if (pCharacteristic == pNameCharacteristic)
    {
        Serial.println("name");
        onNameWrite();
    }
    else if (pCharacteristic == pConnectCharacteristic)
    {
        Serial.println("connect");
        onConnectWrite();
    }
    else if (pCharacteristic == pTypeCharacteristic)
    {
        Serial.println("type");
        onTypeWrite();
    }
    else if (pCharacteristic == pSensorConfigurationCharacteristic)
    {
        Serial.println("sensor configuration");
        onSensorConfigurationWrite();
    }
    else
    {
        Serial.println("unknown");
    }
}
void BLEPeer::onSubscribe(NimBLECharacteristic* pCharacteristic, ble_gap_conn_desc* desc, uint16_t subValue) {
    // may not need this...

    bool didSubscribeUpdate = false;
    bool isSubscribed;
    if (subValue == 0 || subValue == 1) {
        didSubscribeUpdate = true;
        isSubscribed = (subValue == 1);
    }
}

bool BLEPeer::shouldScan = false;
void BLEPeer::updateShouldScan()
{
    bool _shouldScan = false;

    if (ble::isServerConnected)
    {
        for (uint8_t index = 0; !_shouldScan && index < NIMBLE_MAX_CONNECTIONS; index++)
        {
            _shouldScan = _shouldScan || (peers[index].autoConnect && peers[index].pClient == nullptr);
        }
    }
    shouldScan = _shouldScan;
    Serial.printf("should scan: %d\n", shouldScan);
}
unsigned long BLEPeer::lastScanCheck = 0;
void BLEPeer::checkScan()
{
    if (pBLEScan->isScanning() != shouldScan)
    {
        if (shouldScan)
        {
            Serial.println("starting ble scan");
            pBLEScan->start(0, nullptr, false);
        }
        else
        {
            Serial.println("stopping ble scan");
            pBLEScan->stop();
        }
    }
}

unsigned long BLEPeer::currentTime = 0;
void BLEPeer::loop()
{
    currentTime = millis();
    if (currentTime - lastScanCheck > check_scan_interval_ms)
    {
        lastScanCheck = currentTime - (currentTime % check_scan_interval_ms);
        checkScan();
    }

    for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        peers[index]._loop();
    }
}
void BLEPeer::onServerConnect()
{
    updateShouldScan();
}
void BLEPeer::onServerDisconnect()
{
    for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        peers[index].disconnect();
    }

    updateShouldScan();
    checkScan();
}
void BLEPeer::disconnect()
{
    if (pClient != nullptr)
    {
        pClient->disconnect();
        pClient = nullptr;
    }
}

void BLEPeer::onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        if (peers[index].pRemoteSensorDataCharacteristic == pRemoteCharacteristic)
        {
            peers[index]._onRemoteSensorDataCharacteristicNotification(pRemoteCharacteristic, pData, length, isNotify);
            break;
        }
    }
}
void BLEPeer::_onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    if (pRemoteCharacteristic == pRemoteSensorDataCharacteristic)
    {
        sensorDataSize = length;
        memcpy(sensorData, pData, sensorDataSize);
        shouldNotifySensorData = true;
    }
}
void BLEPeer::notifySensorData() {
    pSensorDataCharacteristic->setValue(sensorData, sensorDataSize);
    pSensorDataCharacteristic->notify();
}

bool BLEPeer::isConnected()
{
    Serial.println("checking if connected...");
    return pClient != nullptr && pClient->isConnected();
}
bool BLEPeer::connectToDevice()
{
    Serial.println("connecting to device...");

    pClient = nullptr;

    Serial.println("getting client list size...");
    if (NimBLEDevice::getClientListSize())
    {
        Serial.println("getting client by peer address");
        Serial.println(pAdvertisedDevice != nullptr);
        pClient = NimBLEDevice::getClientByPeerAddress(pAdvertisedDevice->getAddress());
        if (pClient)
        {
            if (!pClient->connect(pAdvertisedDevice, false))
            {
                Serial.println("Reconnect failed");
                return false;
            }
            else
            {
                Serial.println("Reconnected client");
            }
        }
    }

    if (pClient == nullptr)
    {
        if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS)
        {
            Serial.println("Max clients reached - no more connections available");
            return false;
        }

        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(this, false);
        pClient->setConnectionParams(12, 12, 0, 51);
        pClient->setConnectTimeout(5);
        if (!pClient->connect(pAdvertisedDevice))
        {
            pClient = nullptr;
        }
    }

    if (!pClient->isConnected())
    {
        if (!pClient->connect(pAdvertisedDevice))
        {
            Serial.println("Failed to connect");
            return false;
        }
    }

    Serial.print("Connected to: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    Serial.print("RSSI: ");
    Serial.println(pClient->getRssi());

    Serial.println("getting service...");
    pRemoteService = pClient->getService(ble::pService->getUUID());
    if (pRemoteService)
    {
        Serial.println("got service!");

        Serial.println("getting name characteristic...");
        pRemoteNameCharacteristic = pRemoteService->getCharacteristic(bleName::pCharacteristic->getUUID());
        if (pRemoteNameCharacteristic == nullptr) {
            Serial.println("unable to get name characteristic");
            disconnect();
            return false;
        }
        Serial.println("got name characteristic!");

        Serial.println("getting type characteristic...");
        pRemoteTypeCharacteristic = pRemoteService->getCharacteristic(bleType::pCharacteristic->getUUID());
        if (pRemoteTypeCharacteristic == nullptr) {
            Serial.println("unable to get type characteristic");
            disconnect();
            return false;
        }
        Serial.println("got type characteristic!");

        Serial.println("getting sensorConfiguration characteristic...");
        pRemoteSensorConfigurationCharacteristic = pRemoteService->getCharacteristic(bleSensorData::pConfigurationCharacteristic->getUUID());
        if (pRemoteSensorConfigurationCharacteristic == nullptr) {
            Serial.println("unable to get sensor config characteristic");
            disconnect();
            return false;
        }
        Serial.println("got sensorConfiguration characteristic!");

        Serial.println("getting sensorData characteristic...");
        pRemoteSensorDataCharacteristic = pRemoteService->getCharacteristic(bleSensorData::pDataCharacteristic->getUUID());
        if (pRemoteSensorDataCharacteristic == nullptr) {
            Serial.println("unable to get sensor data characteristic");
            disconnect();
            return false;
        }
        Serial.println("got sensorData characteristic!");

        Serial.println("getting name...");
        _name = pRemoteNameCharacteristic->readValue();
        pNameCharacteristic->setValue(_name);
        Serial.printf("got name(%d): %s\n", _name.length(), _name.c_str());

        Serial.println("getting type...");
        _type = (type::Type)pRemoteTypeCharacteristic->readValue().data()[0];
        pTypeCharacteristic->setValue((uint8_t)_type);
        Serial.printf("got type: %d\n", (uint8_t)_type);

        Serial.println("getting sensorConfiguration...");
        auto _sensorConfiguration = pRemoteSensorConfigurationCharacteristic->readValue().data();
        memcpy(sensorConfiguration, _sensorConfiguration, sizeof(sensorConfiguration));
        pSensorConfigurationCharacteristic->setValue(sensorConfiguration, sizeof(sensorConfiguration));
        Serial.println("got sensorConfiguration!");

        pRemoteSensorDataCharacteristic->subscribe(true, onRemoteSensorDataCharacteristicNotification);

        Serial.println("done getting device!");

        updateIsConnectedCharacteristic();
        updateShouldScan();
    }
    else
    {
        Serial.println("remote service not found");
    }

    return true;
}
void BLEPeer::onConnect(NimBLEClient *_pClient)
{
    Serial.printf("[peer #%d] client connected\n", index);
    //pClient->updateConnParams(120, 120, 0, 60);
}
void BLEPeer::onDisconnect(NimBLEClient *_pClient)
{
    Serial.printf("[peer #%d] client disconnect\n", index);
    updateShouldScan();
    updateIsConnectedCharacteristic();
}
void BLEPeer::updateIsConnectedCharacteristic(bool notify)
{
    auto _isConnected = isConnected();
    pIsConnectedCharacteristic->setValue((uint8_t *)&_isConnected, 1);
    if (notify)
    {
        pIsConnectedCharacteristic->notify();
    }
}
bool BLEPeer::onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
{
    if (params->itvl_min < 24)
    { /** 1.25ms units */
        return false;
    }
    else if (params->itvl_max > 40)
    { /** 1.25ms units */
        return false;
    }
    else if (params->latency > 2)
    { /** Number of intervals allowed to skip */
        return false;
    }
    else if (params->supervision_timeout > 100)
    { /** 10ms units */
        return false;
    }

    return true;
}
void BLEPeer::_loop()
{
    if (foundDevice)
    {
        connectToDevice();
        foundDevice = false;
    }

    if (shouldChangeType) {
        changeType();
        shouldChangeType = false;
    }

    if (shouldChangeSensorConfiguration) {
        changeSensorConfiguration();
        shouldChangeSensorConfiguration = false;
    }

    if (shouldChangeName) {
        changeName();
        shouldChangeName = false;
    }

    if (shouldNotifySensorData) {
        notifySensorData();
        shouldNotifySensorData = false;
    }

    if (shouldDisconnect) {
        disconnect();
        shouldDisconnect = false;
    }
}