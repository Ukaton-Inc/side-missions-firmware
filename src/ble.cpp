#include "definitions.h"
#include "ble.h"
#include "name.h"
#include "motion.h"
#include "tflite.h"
#include "fileTransfer.h"
#if IS_INSOLE
#include "pressure.h"
#endif

namespace ble
{
    BLEServer *pServer;
    BLEService *pService;

    bool isServerConnected = false;

    BLEAdvertising *pAdvertising;
    BLEAdvertisementData *pAdvertisementData;

    BLECharacteristic *pErrorMessageCharacteristic;

    unsigned long lastTimeConnected = 0;
    class ServerCallbacks : public BLEServerCallbacks
    {
        void onConnect(BLEServer *pServer)
        {
            isServerConnected = true;
            Serial.println("connected");

            motion::start();
        };

        void onDisconnect(BLEServer *pServer)
        {
            lastTimeConnected = millis();
            isServerConnected = false;
            Serial.println("disconnected");

            motion::stop();
#ifdef _PRESSURE_
            pressure::stop();
#endif
#ifdef _TF_LITE_
            tfLite::stop();
#endif
#ifdef _FILE_TRANSFER_
            fileTransfer::cancelFileTransfer();
#endif
        }
    };

    void setup()
    {
        BLEDevice::init(DEFAULT_BLE_NAME);
        pServer = BLEDevice::createServer();
        pServer->setCallbacks(new ServerCallbacks());
        pService = pServer->createService(BLEUUID(GENERATE_UUID("0000")), 256);

        pAdvertising = pServer->getAdvertising();
        pAdvertising->addServiceUUID(pService->getUUID());
        pAdvertising->setScanResponse(true);
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
        pAdvertising->start();
    }
} // namespace ble