#include "ble.h"
#include "name.h"
#include "imu.h"

namespace ble
{
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
            Serial.println("connected");

            imu::start();
        };

        void onDisconnect(BLEServer *pServer)
        {
            isServerConnected = false;
            Serial.println("disconnected");
            
            imu::stop();
            // clear tinyML bitmask

            pAdvertising->start();
        }
    };

    void setup()
    {
        BLEDevice::init(DEFAULT_BLE_NAME);
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks());
        pService = pServer->createService(BLEUUID(GENERATE_UUID("0000")), 100);
        
        pAdvertising = pServer->getAdvertising();
        pAdvertising->addServiceUUID(pService->getUUID());
        pAdvertising->setScanResponse(true);
        pAdvertising->setMinPreferred(0x06);
        pAdvertising->setMinPreferred(0x12);

        pAdvertisementData = new BLEAdvertisementData();
        pAdvertisementData->setCompleteServices(pService->getUUID());
        pAdvertising->setAdvertisementData(*pAdvertisementData);
    }

    BLECharacteristic *createCharacteristic(const char *uuid, uint32_t properties, const char *name, BLEService *_pService)
    {
        BLECharacteristic *pCharacteristic = _pService->createCharacteristic(uuid, properties);
        BLEDescriptor *pDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
        pDescriptor->setValue(name);
        pCharacteristic->addDescriptor(pDescriptor);
        pCharacteristic->addDescriptor(new BLE2902());
        return pCharacteristic;
    }

    void start()
    {
        pService->start();

        pAdvertisementData->setName(name::name);
        pAdvertisementData->setShortName(name::name);
        
        pAdvertising->start();
    }
} // namespace ble