#include "definitions.h"
#include "ble.h"

#include "bleDebug.h"
#include "bleErrorMessage.h"
#include "information/bleType.h"
#include "information/bleName.h"
#include "sensor/bleMotionCalibration.h"
#include "sensor/bleSensorData.h"
#include "weight/bleWeightData.h"
#include "bleBattery.h"
#include "wifi/bleWifi.h"

namespace ble
{
    unsigned long lastTimeConnected = 0;

    BLEServer *pServer;
    BLEService *pService;

    BLEAdvertising *pAdvertising;
    BLEAdvertisementData *pAdvertisementData;

    bool isServerConnected = false;
    class ServerCallbacks : public BLEServerCallbacks
    {
        void onConnect(BLEServer *pServer)
        {
            isServerConnected = true;
            Serial.println("connected via ble");
            bleSensorData::updateConfigurationCharacteristic();
        };

        void onDisconnect(BLEServer *pServer)
        {
            lastTimeConnected = millis();
            isServerConnected = false;
            Serial.println("disconnected via ble");
            bleSensorData::clearConfigurations();
            bleWeightData::clearDelay();
        }
    };

    void setup()
    {
        BLEDevice::init(bleName::getName()->c_str());
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks());
        pService = pServer->createService(BLEUUID(GENERATE_UUID("0000")), 256);

        pAdvertising = pServer->getAdvertising();
        pAdvertising->addServiceUUID(pService->getUUID());
        pAdvertising->setScanResponse(true);

        bleDebug::setup();
        bleErrorMessage::setup();
        bleType::setup();
        bleName::setup();
        bleMotionCalibration::setup();
        bleSensorData::setup();
        bleWeightData::setup();
        bleWifi::setup();
        bleBattery::setup();

        start();
    }

    BLECharacteristic *createCharacteristic(const char *uuid, uint32_t properties, const char *name, BLEService *_pService)
    {
        BLECharacteristic *pCharacteristic = _pService->createCharacteristic(uuid, properties);

        BLEDescriptor *pNameDescriptor = pCharacteristic->createDescriptor(NimBLEUUID((uint16_t)0x2901), NIMBLE_PROPERTY::READ);
        pNameDescriptor->setValue((uint8_t *)name, strlen(name));

        return pCharacteristic;
    }
    BLECharacteristic *createCharacteristic(BLEUUID uuid, uint32_t properties, const char *name, BLEService *_pService)
    {
        BLECharacteristic *pCharacteristic = _pService->createCharacteristic(uuid, properties);

        BLEDescriptor *pNameDescriptor = pCharacteristic->createDescriptor(NimBLEUUID((uint16_t)0x2901), NIMBLE_PROPERTY::READ);
        pNameDescriptor->setValue((uint8_t *)name, strlen(name));

        return pCharacteristic;
    }

    void start()
    {
        pService->start();
        pServer->startAdvertising();
        Serial.println("starting ble...");
    }

    void loop() {
        if (isServerConnected) {
            bleMotionCalibration::loop();
            bleSensorData::loop();
            bleWeightData::loop();
            bleBattery::loop();
            bleWifi::loop();
        }
    }
} // namespace ble