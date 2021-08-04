#include "definitions.h"
#include "ble.h"
#include "name.h"
#if IS_INSOLE
    #include "pressure.h"
    #include "crappyMotion.h"
#else
    #include "motion.h"
    #include "tflite.h"
    #include "fileTransfer.h"
#endif

namespace ble
{
    BLEServer *pServer;
    BLEService *pService;

    bool isServerConnected = false;

    BLEAdvertising *pAdvertising;
    BLEAdvertisementData *pAdvertisementData;

    BLECharacteristic *pErrorMessageCharacteristic;

    class ServerCallbacks : public BLEServerCallbacks
    {
        void onConnect(BLEServer *pServer)
        {
            isServerConnected = true;
            Serial.println("connected");
            
            #ifdef _MOTION_
                motion::start();
            #endif
        };

        void onDisconnect(BLEServer *pServer)
        {
            isServerConnected = false;
            Serial.println("disconnected");
            
            #ifdef _MOTION_
                motion::stop();
            #endif
            #ifdef _CRAPPY_MOTION_
                crappyMotion::stop();
            #endif
            #ifdef _PRESSURE_
                pressure::stop();
            #endif
            #ifdef _TF_LITE_
                tfLite::stop();
            #endif
            #ifdef _FILE_TRANSFER_
                fileTransfer::cancelFileTransfer();
            #endif
            pAdvertising->start();
        }
    };

    void setup()
    {
        BLEDevice::init(DEFAULT_BLE_NAME);
        //BLEDevice::setMTU(max_characteristic_value_length);
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