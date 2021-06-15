#define DEVICE_NAME "Ukaton Side Mission 1" // replace number with your module's number

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <lwipopts.h>

Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);

bool isBnoAwake = false;

#define ACCELERATION_CONFIGURATION_BIT_INDEX 4
#define LINEAR_ACCELERATION_CONFIGURATION_BIT_INDEX 5
#define ROTATION_RATE_CONFIGURATION_BIT_INDEX 6
#define QUATERNION_CONFIGURATION_BIT_INDEX 7

bool sendAccelerationData = false;
bool sendLinearAccelerationData = false;
bool sendRotationRateData = false;
bool sendQuaternionData = false;

imu::Vector<3> accelerationVector;
imu::Vector<3> linearAccelerationVector;
imu::Vector<3> rotationRateVector;
imu::Quaternion quaternion;

#define DATA_COMPONENT_SIZE 4
#define TIMESTAMP_SIZE 4

#define ACCELERATION_DATA_SIZE (3*DATA_COMPONENT_SIZE)+TIMESTAMP_SIZE
#define LINEAR_ACCELERATION_DATA_SIZE (3*DATA_COMPONENT_SIZE)+TIMESTAMP_SIZE
#define ROTATION_RATE_DATA_SIZE (3*DATA_COMPONENT_SIZE)+TIMESTAMP_SIZE
#define QUATERNION_DATA_SIZE (4*DATA_COMPONENT_SIZE)+TIMESTAMP_SIZE

uint8_t accelerationData[ACCELERATION_DATA_SIZE];
uint8_t linearAcclerationData[LINEAR_ACCELERATION_DATA_SIZE];
uint8_t rotationRateData[ROTATION_RATE_DATA_SIZE];
uint8_t quaternionData[QUATERNION_DATA_SIZE];

float accelerationX = 0;
float accelerationY = 0;
float accelerationZ = 0;

float linearAccelerationX = 0;
float linearAccelerationY = 0;
float linearAccelerationZ = 0;

float rotationRateX = 0;
float rotationRateY = 0;
float rotationRateZ = 0;

float quaternionX = 0;
float quaternionY = 0;
float quaternionZ = 0;
float quaternionW = 0;

#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Arduino.h>

#define DATA_SERVICE_UUID "a84a7896-9d7a-41c8-8d46-7d530266c930"
#define ACCELERATION_DATA_CHARACTERISTIC_UUID "71eb1b54-f492-42bf-b31c-abd6d4a78626"
#define LINEAR_ACCELERATION_DATA_CHARACTERISTIC_UUID "3011e95c-f428-4ecc-b8fe-b478c0ffc9af"
#define ROTATION_RATE_DATA_CHARACTERISTIC_UUID "ff5ca46e-d2ac-4fdb-bb1d-d398f15df5a9"
 
#define CONFIGURATION_SERVICE_UUID "ca51b65e-1c92-4e54-9bd7-fc1088f48832"
#define CONFIGURATION_CHARACTERISTIC_UUID "816ad53c-29df-4699-b25a-4acdf89699d6"
#define QUATERNION_DATA_CHARACTERISTIC_UUID "bb52dc35-1a47-41c1-ae97-ce138dbf2cab"

BLEServer *pServer;

BLEService *pDataService;
BLECharacteristic *pAccelerationDataCharacteristic;
BLECharacteristic *pLinearAccelerationDataCharacteristic;
BLECharacteristic *pRotationRateDataCharacteristic;

BLEService *pConfigurationService;
BLECharacteristic *pConfigurationCharacteristic;
BLECharacteristic *pQuaternionDataCharacteristic;

bool isServerConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      isServerConnected = true;
      Serial.println("connected");
    };

    void onDisconnect(BLEServer* pServer) {
      isServerConnected = false;
      pServer->getAdvertising()->start();
      Serial.println("disconnected");

      sendAccelerationData = false;
      sendLinearAccelerationData = false;
      sendRotationRateData = false;
      sendQuaternionData = false;

      bno.enterSuspendMode();
      isBnoAwake = false;
    }
};

class ConfigurationCharacteristicCallbacks: public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic *pCharacteristic) { 
      uint8_t* configurationData = pCharacteristic->getData();

      if (sizeof(configurationData) >= 1) {
        uint8_t configurationByte = configurationData[0];

        if (configurationByte == 1) {
            sendAccelerationData = false;
            sendLinearAccelerationData = false;
            sendRotationRateData = false;
            sendQuaternionData = false;

            bno.enterSuspendMode();
            isBnoAwake = false;
        }
        else {
            sendAccelerationData = bitRead(configurationByte, ACCELERATION_CONFIGURATION_BIT_INDEX);
            sendLinearAccelerationData = bitRead(configurationByte, LINEAR_ACCELERATION_CONFIGURATION_BIT_INDEX);
            sendRotationRateData = bitRead(configurationByte, ROTATION_RATE_CONFIGURATION_BIT_INDEX);
            sendQuaternionData = bitRead(configurationByte, QUATERNION_CONFIGURATION_BIT_INDEX);

            if (!isBnoAwake) {
              bno.enterNormalMode();
              isBnoAwake = true;
            }
        }
      }
    }
};

void setup() {
  Serial.begin(115200);

  if(!bno.begin()) {
      Serial.println("No BNO055 detected");
  }
  delay(1000);
  bno.setExtCrystalUse(false);
  bno.enterSuspendMode();

  BLEDevice::init(DEVICE_NAME);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  pDataService = pServer->createService(DATA_SERVICE_UUID);

  pAccelerationDataCharacteristic = pDataService->createCharacteristic(
    ACCELERATION_DATA_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  BLEDescriptor *accelerationDataDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  accelerationDataDescriptor->setValue("Acceleration Data");
  pAccelerationDataCharacteristic->addDescriptor(accelerationDataDescriptor);
  pAccelerationDataCharacteristic->addDescriptor(new BLE2902());

  pLinearAccelerationDataCharacteristic = pDataService->createCharacteristic(
    LINEAR_ACCELERATION_DATA_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  BLEDescriptor *linearAccelerationDataDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  linearAccelerationDataDescriptor->setValue("Linear Acceleration Data");
  pLinearAccelerationDataCharacteristic->addDescriptor(linearAccelerationDataDescriptor);
  pLinearAccelerationDataCharacteristic->addDescriptor(new BLE2902());

  pRotationRateDataCharacteristic = pDataService->createCharacteristic(
    ROTATION_RATE_DATA_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  BLEDescriptor *rotationRateDataDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  rotationRateDataDescriptor->setValue("Rotation Rate Data");
  pRotationRateDataCharacteristic->addDescriptor(rotationRateDataDescriptor);
  pRotationRateDataCharacteristic->addDescriptor(new BLE2902());

  pDataService->start();

  pConfigurationService = pServer->createService(CONFIGURATION_SERVICE_UUID);

  pQuaternionDataCharacteristic = pConfigurationService->createCharacteristic(
    QUATERNION_DATA_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  BLEDescriptor *quaternionDataDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  quaternionDataDescriptor->setValue("Quaternion Data");
  pQuaternionDataCharacteristic->addDescriptor(quaternionDataDescriptor);
  pQuaternionDataCharacteristic->addDescriptor(new BLE2902());

  pConfigurationCharacteristic = pConfigurationService->createCharacteristic(
    CONFIGURATION_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pConfigurationCharacteristic->setCallbacks(new ConfigurationCharacteristicCallbacks());
  BLEDescriptor *configurationDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  configurationDescriptor->setValue("Configuation");
  pConfigurationCharacteristic->addDescriptor(configurationDescriptor);
  pConfigurationCharacteristic->addDescriptor(new BLE2902());

  pConfigurationService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(CONFIGURATION_SERVICE_UUID);

  BLEAdvertisementData *pAdvertisementData = new BLEAdvertisementData();
  pAdvertisementData->setCompleteServices(BLEUUID::fromString(CONFIGURATION_SERVICE_UUID));
  pAdvertisementData->setName(DEVICE_NAME);
  pAdvertisementData->setShortName(DEVICE_NAME);
  pAdvertising->setAdvertisementData(*pAdvertisementData);

  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
}

#define BNO055_SAMPLERATE_DELAY_MS (40)
unsigned long currentTime = 0;
unsigned long lastTime = 0;

void loop() {
  currentTime = millis();
  if(currentTime >= lastTime + BNO055_SAMPLERATE_DELAY_MS) {
      lastTime += BNO055_SAMPLERATE_DELAY_MS;

      if (isServerConnected && isBnoAwake) {
        if (sendAccelerationData) {
          accelerationVector = bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);

          accelerationX = float(accelerationVector.x());
          accelerationY = float(accelerationVector.y());
          accelerationZ = float(accelerationVector.z());

          MEMCPY(&accelerationData[DATA_COMPONENT_SIZE*0], &accelerationX, DATA_COMPONENT_SIZE);
          MEMCPY(&accelerationData[DATA_COMPONENT_SIZE*1], &accelerationY, DATA_COMPONENT_SIZE);
          MEMCPY(&accelerationData[DATA_COMPONENT_SIZE*2], &accelerationZ, DATA_COMPONENT_SIZE);

          MEMCPY(&accelerationData[DATA_COMPONENT_SIZE*3], &currentTime, sizeof(currentTime));

          pAccelerationDataCharacteristic->setValue((uint8_t *)accelerationData, sizeof(uint8_t)*ACCELERATION_DATA_SIZE);
          pAccelerationDataCharacteristic->notify();
        }

        if (sendLinearAccelerationData) {
          linearAccelerationVector = bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);

          linearAccelerationX = float(linearAccelerationVector.x());
          linearAccelerationY = float(linearAccelerationVector.y());
          linearAccelerationZ = float(linearAccelerationVector.z());

          MEMCPY(&linearAcclerationData[DATA_COMPONENT_SIZE*0], &linearAccelerationX, DATA_COMPONENT_SIZE);
          MEMCPY(&linearAcclerationData[DATA_COMPONENT_SIZE*1], &linearAccelerationY, DATA_COMPONENT_SIZE);
          MEMCPY(&linearAcclerationData[DATA_COMPONENT_SIZE*2], &linearAccelerationZ, DATA_COMPONENT_SIZE);

          MEMCPY(&linearAcclerationData[DATA_COMPONENT_SIZE*3], &currentTime, sizeof(currentTime));

          pLinearAccelerationDataCharacteristic->setValue((uint8_t *)linearAcclerationData, sizeof(uint8_t)*LINEAR_ACCELERATION_DATA_SIZE);
          pLinearAccelerationDataCharacteristic->notify();
        }
        if (sendRotationRateData) {
          rotationRateVector = bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);

          rotationRateX = float(rotationRateVector.x());
          rotationRateY = float(rotationRateVector.y());
          rotationRateZ = float(rotationRateVector.z());

          MEMCPY(&rotationRateData[DATA_COMPONENT_SIZE*0], &rotationRateX, DATA_COMPONENT_SIZE);
          MEMCPY(&rotationRateData[DATA_COMPONENT_SIZE*1], &rotationRateY, DATA_COMPONENT_SIZE);
          MEMCPY(&rotationRateData[DATA_COMPONENT_SIZE*2], &rotationRateZ, DATA_COMPONENT_SIZE);

          MEMCPY(&rotationRateData[DATA_COMPONENT_SIZE*3], &currentTime, sizeof(currentTime));

          pRotationRateDataCharacteristic->setValue((uint8_t *)rotationRateData, sizeof(uint8_t)*ROTATION_RATE_DATA_SIZE);
          pRotationRateDataCharacteristic->notify();
        }
        if (sendQuaternionData) {
          quaternion = bno.getQuat();

          quaternionW = float(quaternion.w());
          quaternionX = float(quaternion.x());
          quaternionY = float(quaternion.y());
          quaternionZ = float(quaternion.z());

          MEMCPY(&quaternionData[DATA_COMPONENT_SIZE*0], &quaternionW, DATA_COMPONENT_SIZE);
          MEMCPY(&quaternionData[DATA_COMPONENT_SIZE*1], &quaternionX, DATA_COMPONENT_SIZE);
          MEMCPY(&quaternionData[DATA_COMPONENT_SIZE*2], &quaternionY, DATA_COMPONENT_SIZE);
          MEMCPY(&quaternionData[DATA_COMPONENT_SIZE*3], &quaternionZ, DATA_COMPONENT_SIZE);

          MEMCPY(&quaternionData[DATA_COMPONENT_SIZE*4], &currentTime, sizeof(currentTime));

          pQuaternionDataCharacteristic->setValue((uint8_t *)quaternionData, sizeof(uint8_t)*QUATERNION_DATA_SIZE);
          pQuaternionDataCharacteristic->notify();
        }
      }
  }
}