#include "BLEPeer_.h"
#include "information/name.h"
#include "information/bleName.h"
#include "information/bleType.h"
#include "sensor/bleSensorData.h"

BLEPeer_ BLEPeer_::peers[NIMBLE_MAX_CONNECTIONS];

BLEScan *BLEPeer_::pBLEScan;
void BLEPeer_::AdvertisedDeviceCallbacks::onResult(NimBLEAdvertisedDevice *advertisedDevice)
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

void BLEPeer_::formatBLECharacteristicUUID(char *uuidBuffer, uint8_t value)
{
    snprintf(uuidBuffer, BLE_UUID_LENGTH + 1, "%s%s%d%d%s", UUID_PREFIX, BLE_PEER_UUID_PREFIX, index, value, UUID_SUFFIX);
}
void BLEPeer_::formatBLECharacteristicName(char *nameBuffer, const char *_name)
{
    snprintf(nameBuffer, MAX_BLE_ATTRIBUTE_LENGTH + 1, "peer #%d %s", index, _name);
}

void BLEPeer_::setup()
{
    for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        peers[index]._setup(index);
    }

    pBLEScan = BLEDevice::getScan();
    pBLEScan->setActiveScan(true);
    pBLEScan->setAdvertisedDeviceCallbacks(new BLEPeer_::AdvertisedDeviceCallbacks(), false);

    updateShouldScan();
}
void BLEPeer_::_setup(uint8_t _index)
{
    index = _index;

    char preferencesKey[BLE_UUID_LENGTH + 1];
    snprintf(preferencesKey, sizeof(preferencesKey), "BLEPeer_%d", index);
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
    pSensorConfigurationCharacteristic->setValue((uint8_t *) configurations.flattened.data(), configurations.flattened.max_size());
    pSensorConfigurationCharacteristic->setCallbacks(this);

    formatBLECharacteristicUUID(uuidBuffer, 5);
    formatBLECharacteristicName(nameBuffer, "sensor data");
    pSensorDataCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::NOTIFY, nameBuffer);
}
BLEPeer_::~BLEPeer_()
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

void BLEPeer_::onNameWrite()
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
void BLEPeer_::changeName() {
    Serial.printf("setting remote name to %s\n", _name.c_str());
    pRemoteNameCharacteristic->writeValue(_name, true);

    _name = pRemoteNameCharacteristic->readValue();
    Serial.printf("updated remote name: %s\n", _name.c_str());

    pNameCharacteristic->setValue(_name);
}

void BLEPeer_::onConnectWrite()
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

void BLEPeer_::onTypeWrite()
{
    if (isConnected()) {
        typeToChangeTo = (type::Type) pTypeCharacteristic->getValue().data()[0];
        shouldChangeType = true;
    }
}
void BLEPeer_::changeType() {
    Serial.printf("writing remote type %d\n", (uint8_t) typeToChangeTo);
    pRemoteTypeCharacteristic->writeValue((uint8_t)typeToChangeTo, true);
    Serial.println("wrote remote type!");
    _type = (type::Type) pRemoteTypeCharacteristic->readValue().data()[0];
    Serial.println((uint8_t)_type);
    Serial.printf("read remote type %d\n", (uint8_t) _type);
    pTypeCharacteristic->setValue((uint8_t) _type);
    Serial.println("set value!");
}

void BLEPeer_::onSensorConfigurationWrite()
{
    if (isConnected()) {
        auto newConfigurations = pSensorConfigurationCharacteristic->getValue();
        sensorData::setConfigurations((uint8_t *) newConfigurations.data(), newConfigurations.length(), configurations);
        receivedConfiguration = newConfigurations;
        shouldChangeSensorConfiguration = true;
    }
}
void BLEPeer_::changeSensorConfiguration() {
    //pRemoteSensorConfigurationCharacteristic->writeValue(sensorConfiguration, sizeof(sensorConfiguration), true);
    pRemoteSensorConfigurationCharacteristic->writeValue(receivedConfiguration, true);

    /*
    auto _sensorConfiguration = pRemoteSensorConfigurationCharacteristic->readValue().data();
    memcpy(sensorConfiguration, _sensorConfiguration, sizeof(sensorConfiguration));
    pSensorConfigurationCharacteristic->setValue((uint8_t *) sensorConfiguration, sizeof(sensorConfiguration));
    */
}

void BLEPeer_::onWrite(BLECharacteristic *pCharacteristic)
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

bool BLEPeer_::shouldScan = false;
void BLEPeer_::updateShouldScan()
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
unsigned long BLEPeer_::lastScanCheck = 0;
void BLEPeer_::checkScan()
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

unsigned long BLEPeer_::currentTime = 0;
void BLEPeer_::loop()
{
    currentTime = millis();

    if (isServerConnected != ble::isServerConnected) {
        isServerConnected = ble::isServerConnected;
        if (isServerConnected) {
            onServerConnect();
        }
        else {
            onServerDisconnect();
        }
    }

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

bool BLEPeer_::isServerConnected = false;
void BLEPeer_::onServerConnect()
{
    updateShouldScan();
}
void BLEPeer_::onServerDisconnect()
{
    for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        peers[index].shouldDisconnect = true;
    }

    updateShouldScan();
}
void BLEPeer_::disconnect()
{
    if (pClient != nullptr)
    {
        pClient->disconnect();
        pClient = nullptr;
    }
}

void BLEPeer_::onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
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
void BLEPeer_::_onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic *pRemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
    if (pRemoteCharacteristic == pRemoteSensorDataCharacteristic)
    {
        sensorDataSize = length;
        memcpy(sensorData, pData, sensorDataSize);
        shouldNotifySensorData = true;
    }
}
void BLEPeer_::notifySensorData() {
    pSensorDataCharacteristic->setValue(sensorData, sensorDataSize);
    pSensorDataCharacteristic->notify();
}

bool BLEPeer_::isConnected()
{
    Serial.println("checking if is connected");
    return pClient != nullptr && pClient->isConnected();
}
bool BLEPeer_::connectToDevice()
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
        auto sensorConfiguration = pRemoteSensorConfigurationCharacteristic->readValue();
        pSensorConfigurationCharacteristic->setValue(sensorConfiguration);
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
void BLEPeer_::onConnect(NimBLEClient *_pClient)
{
    Serial.printf("[peer #%d] client connected\n", index);
    //pClient->updateConnParams(120, 120, 0, 60);
}
void BLEPeer_::onDisconnect(NimBLEClient *_pClient)
{
    Serial.printf("[peer #%d] client disconnect\n", index);
    updateShouldScan();
    shouldUpdateIsConnected = true;
}
void BLEPeer_::updateIsConnectedCharacteristic(bool notify)
{   
    Serial.println("will update isConnectedCharacteristic");
    auto _isConnected = isConnected();
    pIsConnectedCharacteristic->setValue((uint8_t *)&_isConnected, 1);
    if (notify)
    {
        pIsConnectedCharacteristic->notify();
    }
}
bool BLEPeer_::onConnParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
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
void BLEPeer_::_loop()
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

    if (shouldUpdateIsConnected) {
        updateIsConnectedCharacteristic();
        shouldUpdateIsConnected = false;
    }
}