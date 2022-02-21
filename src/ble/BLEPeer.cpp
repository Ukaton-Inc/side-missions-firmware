#include "BLEPeer.h"
#include "information/name.h"
#include "information/bleName.h"
#include "information/bleType.h"
#include "sensor/bleSensorData.h"

BLEPeer BLEPeer::peers[NIMBLE_MAX_CONNECTIONS];

BLEScan *BLEPeer::pBLEScan;
void BLEPeer::AdvertisedDeviceCallbacks::onResult(NimBLEAdvertisedDevice *advertisedDevice)
{
    if (advertisedDevice->getServiceUUID() == ble::pService->getUUID()) {
        Serial.printf("found device \"%s\"\n", advertisedDevice->getName().c_str());

        for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
        {
            if (peers[index].autoConnect && (peers[index].pClient == nullptr || !peers[index].pClient->isConnected()) && peers[index].name == advertisedDevice->getName()) {
                peers[index].pAdvertisedDevice = advertisedDevice;
                peers[index].foundDevice = true;
                break;
            }
        }
    }
}

void BLEPeer::formatBLECharacteristicUUID(char *uuidBuffer, uint8_t value)
{
    snprintf(uuidBuffer, BLE_UUID_LENGTH + 1, "%s%s%d%d%s", UUID_PREFIX, BLE_PEER_UUID_PREFIX, index, value, UUID_SUFFIX);
}
void BLEPeer::formatBLECharacteristicName(char *nameBuffer, const char *name)
{
    snprintf(nameBuffer, MAX_BLE_ATTRIBUTE_LENGTH + 1, "peer #%d %s", index, name);
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
        name = preferences.getString("name").c_str();
    }
    pNameCharacteristic->setValue(name);
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
    pIsConnectedCharacteristic->setValue((uint8_t *)&isConnected, 1);

    formatBLECharacteristicUUID(uuidBuffer, 3);
    formatBLECharacteristicName(nameBuffer, "type");
    pTypeCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    pTypeCharacteristic->setValue((uint8_t *)&type, 1);
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

void BLEPeer::onNameWrite() {
    auto _name = pNameCharacteristic->getValue(); 
    if (_name.length() <= name::MAX_NAME_LENGTH) {
        name = _name;
        preferences.putString("name", name.c_str());

        Serial.print("updated name to: ");
        Serial.println(name.c_str());
    }
    else {
        log_e("name's too long");
    }

    pNameCharacteristic->setValue(name);
}
void BLEPeer::onConnectWrite() {
    auto _autoConnect = (bool) pConnectCharacteristic->getValue().data()[0];
    if (_autoConnect != autoConnect) {
        autoConnect = _autoConnect;
        preferences.putBool("connect", autoConnect);

        Serial.print("updated autoConnect to ");
        Serial.println(autoConnect);

        if (!autoConnect && pClient != nullptr) {
            Serial.println("deleting client");
            BLEDevice::deleteClient(pClient);

            pRemoteNameCharacteristic = nullptr;
            pRemoteTypeCharacteristic = nullptr;
            pRemoteSensorConfigurationCharacteristic = nullptr;
            pRemoteSensorDataCharacteristic = nullptr;
            pRemoteService = nullptr;
            pClient = nullptr;
        }

        BLEPeer::updateShouldScan();
    }
}
void BLEPeer::onTypeWrite() {
    // FILL - send to remote char
}
void BLEPeer::onSensorConfigurationWrite() {
    // FILL - send to remote char
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
    else {
        Serial.println("unknown");
    }
}

bool BLEPeer::shouldScan = false;
void BLEPeer::updateShouldScan() {
    bool _shouldScan = false;

    for (uint8_t index = 0; !_shouldScan && index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        _shouldScan = _shouldScan || (peers[index].autoConnect && peers[index].pClient == nullptr);
    }
    shouldScan = _shouldScan;
    Serial.printf("should scan: %d\n", shouldScan);
}
unsigned long BLEPeer::lastScanCheck = 0;
void BLEPeer::checkScan()
{
    if (pBLEScan->isScanning() != shouldScan) {
        if (shouldScan) {
            Serial.println("starting ble scan");
            pBLEScan->start(0, nullptr, false);
        }
        else {
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

void BLEPeer::onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        if (peers[index].pRemoteSensorDataCharacteristic == pRemoteCharacteristic) {
            peers[index]._onRemoteSensorDataCharacteristicNotification(pRemoteCharacteristic, pData, length, isNotify);
            break;
        }
    }
}
void BLEPeer::_onRemoteSensorDataCharacteristicNotification(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    std::string str = (isNotify == true) ? "Notification" : "Indication";
    str += " from ";
    /** NimBLEAddress and NimBLEUUID have std::string operators */
    str += std::string(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
    str += ": Service = " + std::string(pRemoteCharacteristic->getRemoteService()->getUUID());
    str += ", Characteristic = " + std::string(pRemoteCharacteristic->getUUID());
    str += ", Value = " + std::string((char*)pData, length);
    Serial.println(str.c_str());
}

bool BLEPeer::connectToDevice() {
    Serial.println("connecting to device...");

    pClient = nullptr;

    if(NimBLEDevice::getClientListSize()) {
        pClient = NimBLEDevice::getClientByPeerAddress(pAdvertisedDevice->getAddress());
        if(pClient){
            if(!pClient->connect(pAdvertisedDevice, false)) {
                Serial.println("Reconnect failed");
                return false;
            }
            else {
                Serial.println("Reconnected client");
            }
        }
    }

    if (!pClient) {
        if(NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
            Serial.println("Max clients reached - no more connections available");
            return false;
        }

        pClient = NimBLEDevice::createClient();
        pClient->setClientCallbacks(this);
        if (!pClient->connect(pAdvertisedDevice)) {
            pClient = nullptr;
        }
    }

    if(!pClient->isConnected()) {
        if (!pClient->connect(pAdvertisedDevice)) {
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
    if (pRemoteService) {
        Serial.println("got service!");
        
        Serial.println("getting name characteristic...");
        pRemoteNameCharacteristic = pRemoteService->getCharacteristic(bleName::pCharacteristic->getUUID());
        Serial.println("got name characteristic!");

        Serial.println("getting value...");
        name = pRemoteNameCharacteristic->getValue();
        Serial.printf("got name(%d): %s, %s\n", name.length(), name.c_str());

        Serial.println("getting type characteristic...");
        pRemoteTypeCharacteristic = pRemoteService->getCharacteristic(bleType::pCharacteristic->getUUID());
        Serial.println("got type characteristic!");

        Serial.println("getting type...");
        type = (type::Type) pRemoteTypeCharacteristic->getValue().data()[0];
        Serial.printf("got type: %d\n", (uint8_t) type);

        Serial.println("getting sensorConfiguration characteristic...");
        pRemoteSensorConfigurationCharacteristic = pRemoteService->getCharacteristic(bleSensorData::pConfigurationCharacteristic->getUUID());
        Serial.println("got sensorConfiguration characteristic!");

        Serial.println("getting sensorConfiguration...");
        auto _sensorConfiguration = pRemoteSensorConfigurationCharacteristic->getValue().data();
        memcpy(sensorConfiguration, _sensorConfiguration, sizeof(sensorConfiguration));
        Serial.println("got sensorConfiguration!");
        for (uint8_t index = 0; index < sizeof(sensorConfiguration); index++) {
            Serial.printf("%d: %d\n", index, sensorConfiguration[index]);
        }

        Serial.println("getting sensorData characteristic...");
        pRemoteSensorDataCharacteristic = pRemoteService->getCharacteristic(bleSensorData::pDataCharacteristic->getUUID());
        pRemoteSensorDataCharacteristic->subscribe(true, onRemoteSensorDataCharacteristicNotification);
        Serial.println("got sensorData characteristic!");
    }
    else {
        Serial.println("remote service not found");
    }

    return true;
}
void BLEPeer::onConnect(NimBLEClient* pClient) {
    Serial.printf("[peer #%d] client connected\n", index);
    updateShouldScan();
    pClient->updateConnParams(120,120,0,60);
}
void BLEPeer::onDisconnect(NimBLEClient* pClient) {
    Serial.printf("[peer #%d] client disconnect\n", index);
    updateShouldScan();
}
void BLEPeer::_loop()
{
    if (foundDevice) {
        connectToDevice();
        foundDevice = false;
    }
}