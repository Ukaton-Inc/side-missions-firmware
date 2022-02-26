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
    pConnectCharacteristic->setValue((uint8_t *)&autoConnect, 1);
    pConnectCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

    formatBLECharacteristicUUID(uuidBuffer, 2);
    formatBLECharacteristicName(nameBuffer, "isConnected");
    pIsConnectedCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, nameBuffer);
    pIsConnectedCharacteristic->setValue((uint8_t *)&isConnected, 1);

    formatBLECharacteristicUUID(uuidBuffer, 3);
    formatBLECharacteristicName(nameBuffer, "type");
    pTypeCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    pTypeCharacteristic->setValue((uint8_t *)&_type, 1);
    pTypeCharacteristic->setCallbacks(new CharacteristicCallbacks(this));

    formatBLECharacteristicUUID(uuidBuffer, 4);
    formatBLECharacteristicName(nameBuffer, "sensor configuration");
    pSensorConfigurationCharacteristic = ble::createCharacteristic(uuidBuffer, NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, nameBuffer);
    pSensorConfigurationCharacteristic->setValue((uint8_t *) configurations.flattened.data(), sizeof(uint16_t) * configurations.flattened.max_size());
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

unsigned long BLEPeer::currentTime = 0;
void BLEPeer::loop()
{
    currentTime = millis();

    for (uint8_t index = 0; index < NIMBLE_MAX_CONNECTIONS; index++)
    {
        peers[index]._loop();
    }
}
void BLEPeer::_loop()
{

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