#include "BLEPeer.h"
#include "information/name.h"
#include "information/bleName.h"
#include "information/bleType.h"
#include "sensor/bleSensorData.h"

BLEPeer BLEPeer::peers[NIMBLE_MAX_CONNECTIONS];

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
    snprintf(preferencesKey, sizeof(preferencesKey), "BLEPeer%d", index);
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
    pNameCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

    formatBLECharacteristicUUID(uuidBuffer, 1);
    formatBLECharacteristicName(nameBuffer, "connect");
    pConnectCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    if (preferences.isKey("connect"))
    {
        autoConnect = preferences.getBool("connect");
    }
    pConnectCharacteristic->setValue(autoConnect);
    pConnectCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

    formatBLECharacteristicUUID(uuidBuffer, 2);
    formatBLECharacteristicName(nameBuffer, "isConnected");
    pIsConnectedCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, nameBuffer);
    pIsConnectedCharacteristic->setValue(isConnected);

    formatBLECharacteristicUUID(uuidBuffer, 3);
    formatBLECharacteristicName(nameBuffer, "type");
    pTypeCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    pTypeCharacteristic->setValue((uint8_t)_type);
    pTypeCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

    formatBLECharacteristicUUID(uuidBuffer, 4);
    formatBLECharacteristicName(nameBuffer, "sensor configuration");
    pSensorConfigurationCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    pSensorConfigurationCharacteristic->setValue((uint8_t *)configurations.flattened.data(), sizeof(uint16_t) * configurations.flattened.max_size());
    pSensorConfigurationCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

    formatBLECharacteristicUUID(uuidBuffer, 5);
    formatBLECharacteristicName(nameBuffer, "sensor data");
    pSensorDataCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::NOTIFY, nameBuffer);
}

void BLEPeer::formatBLECharacteristicUUID(char *uuidBuffer, uint8_t value)
{
    snprintf(uuidBuffer, BLE_UUID_LENGTH + 1, "%s%s%d%d%s", UUID_PREFIX, BLE_PEER_UUID_PREFIX, index, value, UUID_SUFFIX);
}
void BLEPeer::formatBLECharacteristicName(char *nameBuffer, const char *_name)
{
    snprintf(nameBuffer, MAX_BLE_ATTRIBUTE_LENGTH + 1, "peer #%d %s", index, _name);
}

bool BLEPeer::isServerConnected = false;
unsigned long BLEPeer::lastServerConnectionCheck = 0;
void BLEPeer::checkServerConnection()
{
    if (isServerConnected != ble::isServerConnected)
    {
        isServerConnected = ble::isServerConnected;
        onServerConnectionUpdate();
    }
}
void BLEPeer::onServerConnectionUpdate()
{
    if (isServerConnected)
    {
        onServerConnect();
    }
    else
    {
        onServerDisconnect();
    }
    updateShouldScan();
}
void BLEPeer::onServerConnect()
{
}
void BLEPeer::onServerDisconnect()
{
    for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        peers[index].shouldDisconnect = true;
    }
}

BLEScan *BLEPeer::pBLEScan;
bool BLEPeer::shouldScan = false;
void BLEPeer::updateShouldScan()
{
    bool _shouldScan = false;

    if (isServerConnected)
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

void BLEPeer::onAdvertisedDevice(BLEAdvertisedDevice *advertisedDevice)
{
    if (advertisedDevice->getServiceUUID() == ble::pService->getUUID())
    {
        Serial.printf("found device \"%s\"\n", advertisedDevice->getName().c_str());

        for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
        {
            if (peers[index].autoConnect && !peers[index].isConnected && peers[index]._name == advertisedDevice->getName())
            {
                Serial.printf("found device for peer #%d!\n", index);
                peers[index].pAdvertisedDevice = advertisedDevice;
                peers[index].shouldConnect = true;
                // FIX - stop scanning...
                updateShouldScan();
                break;
            }
        }
    }
}

void BLEPeer::onClientConnect(NimBLEClient *pClient)
{
}
void BLEPeer::onClientDisconnect(NimBLEClient *pClient)
{
    isConnected = false;
    connectionUpdated = true;
}
void BLEPeer::onClientConnectionUpdate()
{
    pIsConnectedCharacteristic->setValue(isConnected);
    if (pIsConnectedCharacteristic->getSubscribedCount() > 0)
    {
        pIsConnectedCharacteristic->notify();
    }
}
bool BLEPeer::onClientConnectionParamsUpdateRequest(NimBLEClient *pClient, const ble_gap_upd_params *params)
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

void BLEPeer::onCharacteristicWrite(BLECharacteristic *pCharacteristic)
{
    Serial.printf("[peer #%d] wrote to characteristic: ", index);
    if (pCharacteristic == pNameCharacteristic)
    {
        Serial.println("name");
        onNameCharacteristicWrite();
    }
    else if (pCharacteristic == pConnectCharacteristic)
    {
        Serial.println("connect");
        onConnectCharacteristicWrite();
    }
    else if (pCharacteristic == pTypeCharacteristic)
    {
        Serial.println("type");
        onTypeCharacteristicWrite();
    }
    else if (pCharacteristic == pSensorConfigurationCharacteristic)
    {
        Serial.println("sensor configuration");
        onSensorConfigurationCharacteristicWrite();
    }
    else
    {
        Serial.println("unknown");
    }
}
void BLEPeer::onNameCharacteristicWrite()
{
    auto newName = pNameCharacteristic->getValue();
    if (newName != _name)
    {
        if (name::isNameValid(newName.c_str(), newName.length()))
        {
            _name = newName;
            preferences.putString("name", _name.c_str());

            Serial.printf("updated name to: %s\n", _name.c_str());

            if (isConnected)
            {
                shouldChangeRemoteNameCharacteristic = true;
            }
        }
        else
        {
            Serial.println("invalid name");
        }

        pNameCharacteristic->setValue(_name);
    }
}
void BLEPeer::onConnectCharacteristicWrite()
{
    auto newAutoConnect = (bool)pConnectCharacteristic->getValue().data()[0];
    if (newAutoConnect != autoConnect)
    {
        autoConnect = newAutoConnect;
        preferences.putBool("connect", autoConnect);

        Serial.printf("updated autoConnect to %d\n", autoConnect);
        shouldChangeConnection = true;
        updateShouldScan();
    }
}
void BLEPeer::onTypeCharacteristicWrite()
{
    if (!isConnected)
    {
        return;
    }

    auto newType = (type::Type)pTypeCharacteristic->getValue().data()[0];
    if (newType != _type)
    {
        if (type::isTypeValid(newType))
        {
            _type = newType;
            shouldChangeRemoteTypeCharacteristic = true;
        }
        else
        {
            Serial.println("invalid type");
        }

        pTypeCharacteristic->setValue((uint8_t *)&_type, 1);
    }
}
void BLEPeer::onSensorConfigurationCharacteristicWrite()
{
    if (!isConnected)
    {
        return;
    }

    sensorConfigurationCharacteristicValue = pSensorConfigurationCharacteristic->getValue();
    sensorData::setConfigurations((uint8_t *)sensorConfigurationCharacteristicValue.data(), sensorConfigurationCharacteristicValue.length(), configurations);
    shouldChangeRemoteSensorConfigurationCharacteristic = true;
}

void BLEPeer::changeRemoteNameCharacteristic()
{
    Serial.printf("setting remote name to %s\n", _name.c_str());
    pRemoteNameCharacteristic->writeValue(_name, true);

    _name = pRemoteNameCharacteristic->readValue();
    Serial.printf("updated remote name: %s\n", _name.c_str());

    pNameCharacteristic->setValue(_name);
}
void BLEPeer::changeConnection()
{
    if (!autoConnect && isConnected)
    {
        disconnect();
    }
}
void BLEPeer::changeRemoteTypeCharacteristic()
{
    Serial.printf("writing remote type %d\n", (uint8_t)_type);
    pRemoteTypeCharacteristic->writeValue((uint8_t)_type, true);
    Serial.println("wrote remote type!");

    _type = (type::Type)pRemoteTypeCharacteristic->readValue().data()[0];
    Serial.printf("read remote type %d\n", (uint8_t)_type);

    pTypeCharacteristic->setValue((uint8_t)_type);
    Serial.println("set type value!");
}
void BLEPeer::changeRemoteSensorConfigurationCharacteristic()
{
    Serial.println("writing to remote sensor configuration");
    pRemoteSensorConfigurationCharacteristic->writeValue(sensorConfigurationCharacteristicValue, true);
    Serial.println("wrote to remote sensor configuration!");

    Serial.println("writing to local sensor configuration...");
    sensorConfigurationCharacteristicValue = pRemoteSensorConfigurationCharacteristic->readValue();
    pSensorConfigurationCharacteristic->setValue((uint8_t *)sensorConfigurationCharacteristicValue.data(), sensorConfigurationCharacteristicValue.length());
    Serial.println("updated to local sensor configuration!");
}

bool BLEPeer::connect()
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
        pClient->setClientCallbacks(new ClientCallbacks(this));
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
        if (pRemoteNameCharacteristic == nullptr)
        {
            Serial.println("unable to get name characteristic");
            disconnect();
            return false;
        }
        Serial.println("got name characteristic!");

        Serial.println("getting type characteristic...");
        pRemoteTypeCharacteristic = pRemoteService->getCharacteristic(bleType::pCharacteristic->getUUID());
        if (pRemoteTypeCharacteristic == nullptr)
        {
            Serial.println("unable to get type characteristic");
            disconnect();
            return false;
        }
        Serial.println("got type characteristic!");

        Serial.println("getting sensorConfiguration characteristic...");
        pRemoteSensorConfigurationCharacteristic = pRemoteService->getCharacteristic(bleSensorData::pConfigurationCharacteristic->getUUID());
        if (pRemoteSensorConfigurationCharacteristic == nullptr)
        {
            Serial.println("unable to get sensor config characteristic");
            disconnect();
            return false;
        }
        Serial.println("got sensorConfiguration characteristic!");

        Serial.println("getting sensorData characteristic...");
        pRemoteSensorDataCharacteristic = pRemoteService->getCharacteristic(bleSensorData::pDataCharacteristic->getUUID());
        if (pRemoteSensorDataCharacteristic == nullptr)
        {
            Serial.println("unable to get sensor data characteristic");
            disconnect();
            return false;
        }
        Serial.println("got sensorData characteristic!");

        Serial.println("done getting device!");

        isConnected = true;
        connectionUpdated = true;
    }
    else
    {
        Serial.println("remote service not found");
    }

    return true;
}

void BLEPeer::onConnectionUpdate()
{
    if (isConnected)
    {
        onConnection();
    }
    else
    {
        onDisconnection();
    }
    shouldChangeConnectionCharacteristic = true;
    updateShouldScan();
}
void BLEPeer::onConnection()
{
    Serial.println("getting name...");
    _name = pRemoteNameCharacteristic->readValue();
    shouldChangeNameCharacteristic = true;
    Serial.printf("got name(%d): %s\n", _name.length(), _name.c_str());

    Serial.println("getting type...");
    _type = (type::Type)pRemoteTypeCharacteristic->readValue().data()[0];
    shouldChangeTypeCharacteristic = true;
    Serial.printf("got type: %d\n", (uint8_t)_type);

    Serial.println("getting sensorConfiguration...");
    sensorConfigurationCharacteristicValue = pRemoteSensorConfigurationCharacteristic->readValue();
    shouldChangeSensorConfigurationCharacteristic = true;
    Serial.println("got sensorConfiguration!");

    Serial.println("subscribing to sensorData..");
    pRemoteSensorDataCharacteristic->subscribe(true, onRemoteSensorDataCharacteristicNotification);
    Serial.println("subscribed to sensorData!");
}
void BLEPeer::onDisconnection()
{
    
}

void BLEPeer::changeNameCharacteristic()
{
    pNameCharacteristic->setValue(_name);
}
void BLEPeer::changeTypeCharacteristic()
{
    pTypeCharacteristic->setValue((uint8_t)_type);
}
void BLEPeer::changeConnectionCharacteristic(bool notify)
{
    pIsConnectedCharacteristic->setValue(isConnected);
    if (notify && pIsConnectedCharacteristic->getSubscribedCount() > 0)
    {
        pIsConnectedCharacteristic->notify();
    }
}
void BLEPeer::changeSensorConfigurationCharacteristic()
{
    pSensorConfigurationCharacteristic->setValue(sensorConfigurationCharacteristicValue);
}
void BLEPeer::notifySensorDataCharacteristic()
{
    // FILL
}

void BLEPeer::disconnect()
{
    if (pClient != nullptr)
    {
        pClient->disconnect();
    }

    shouldChangeRemoteNameCharacteristic = false;
    shouldChangeConnection = false;
    shouldChangeRemoteTypeCharacteristic = false;
    shouldChangeRemoteSensorConfigurationCharacteristic = false;
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
        sensorDataCharacteristicValue = pRemoteSensorConfigurationCharacteristic->getValue();
        shouldNotifySensorData = true;
    }
}

unsigned long BLEPeer::currentTime = 0;
void BLEPeer::loop()
{
    currentTime = millis();

    if (currentTime - lastServerConnectionCheck > check_server_connection_interval_ms)
    {
        lastServerConnectionCheck = currentTime - (currentTime % check_server_connection_interval_ms);
        checkServerConnection();
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
void BLEPeer::_loop()
{
    if (shouldConnect)
    {
        connect();
        shouldConnect = false;
    }

    if (shouldDisconnect)
    {
        disconnect();
        shouldDisconnect = false;
    }

    if (connectionUpdated)
    {
        onConnectionUpdate();
        connectionUpdated = false;
    }

    // local characteristic flags
    if (shouldChangeNameCharacteristic)
    {
        changeNameCharacteristic();
        shouldChangeNameCharacteristic = false;
    }

    if (shouldChangeConnectionCharacteristic)
    {
        changeConnectionCharacteristic();
        shouldChangeConnectionCharacteristic = false;
    }

    if (shouldChangeTypeCharacteristic)
    {
        changeTypeCharacteristic();
        shouldChangeTypeCharacteristic = false;
    }

    if (shouldChangeSensorConfigurationCharacteristic)
    {
        changeSensorConfigurationCharacteristic();
        shouldChangeSensorConfigurationCharacteristic = false;
    }

    if (shouldNotifySensorData)
    {
        notifySensorDataCharacteristic();
        shouldNotifySensorData = false;
    }

    // remote characteristic flags
    if (shouldChangeRemoteNameCharacteristic)
    {
        changeRemoteNameCharacteristic();
        shouldChangeRemoteNameCharacteristic = false;
    }

    if (shouldChangeConnection)
    {
        changeConnection();
        shouldChangeConnection = false;
    }

    if (shouldChangeRemoteTypeCharacteristic)
    {
        changeRemoteTypeCharacteristic();
        shouldChangeRemoteTypeCharacteristic = false;
    }

    if (shouldChangeRemoteSensorConfigurationCharacteristic)
    {
        changeRemoteSensorConfigurationCharacteristic();
        shouldChangeRemoteSensorConfigurationCharacteristic = false;
    }
}