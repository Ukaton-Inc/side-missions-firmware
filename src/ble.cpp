#include "ble.h"
#include "sensors.h"
#include "gestures.h"

namespace ble
{
    BLEServer *pServer;
    BLEService *pService;

    bool isServerConnected = false;

    BLEAdvertising *pAdvertising;
    BLEAdvertisementData *pAdvertisementData;

    class ServerCallbacks : public BLEServerCallbacks
    {
        void onConnect(BLEServer *pServer)
        {
            isServerConnected = true;
            Serial.println("connected");

            sensors::start();
        };

        void onDisconnect(BLEServer *pServer)
        {
            isServerConnected = false;
            Serial.println("disconnected");

            sensors::stop();
            gestures::stop();
        }
    };

    void setup()
    {
        BLEDevice::init(DEFAULT_BLE_NAME);
        
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks());
        pService = pServer->createService(BLEUUID("0000fdd2-0000-1000-8000-00805f9b34fb"), 256);

        pAdvertising = pServer->getAdvertising();

        String manufacturerData;
        int manufacturerCode = 0x1601;
        char m2 = (char)(manufacturerCode >> 8);
        manufacturerCode <<= 8;
        char m1 = (char)(manufacturerCode >> 8);
        manufacturerData.concat(m1);
        manufacturerData.concat(m2);

        pAdvertising->setManufacturerData(manufacturerData.c_str());
        pAdvertising->addServiceUUID(pService->getUUID());
        pAdvertising->setScanResponse(true);
    }
    
    BLECharacteristic *createCharacteristic(const char *uuid, uint32_t properties, const char *name, BLEService *_pService)
    {
        BLECharacteristic *pCharacteristic = _pService->createCharacteristic(uuid, properties);

        BLEDescriptor *pNameDescriptor = pCharacteristic->createDescriptor(NimBLEUUID((uint16_t)0x2901), NIMBLE_PROPERTY::READ);
        pNameDescriptor->setValue((uint8_t *) name, strlen(name));
        
        return pCharacteristic;
    }
    BLECharacteristic *createCharacteristic(BLEUUID uuid, uint32_t properties, const char *name, BLEService *_pService)
    {
        BLECharacteristic *pCharacteristic = _pService->createCharacteristic(uuid, properties);

        BLEDescriptor *pNameDescriptor = pCharacteristic->createDescriptor(NimBLEUUID((uint16_t)0x2901), NIMBLE_PROPERTY::READ);
        pNameDescriptor->setValue((uint8_t *) name, strlen(name));
        
        return pCharacteristic;
    }

    void start()
    {
        pService->start();
        pAdvertising->start();
    }
} // namespace ble