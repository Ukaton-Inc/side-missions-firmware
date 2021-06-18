#define DEVICE_NAME "Ukaton Side Mission 3" // replace number with your module's number

#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BNO055.h>
#include <utility/imumaths.h>
#include <lwipopts.h>

Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);

bool isBnoAwake = false;

uint8_t configurationBitmask;
uint16_t dataDelay;

#define ACCELERATION_CONFIGURATION_BIT_INDEX 0
#define GRAVITY_CONFIGURATION_BIT_INDEX 1
#define LINEAR_ACCELERATION_CONFIGURATION_BIT_INDEX 2
#define ROTATION_RATE_CONFIGURATION_BIT_INDEX 3
#define MAGNETOMETER_CONFIGURATION_BIT_INDEX 4
#define QUATERNION_CONFIGURATION_BIT_INDEX 5

#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Arduino.h>

#define MAX_CHARACTERISTIC_VALUE_LENGTH 23
#define DATA_CHARACTERISTIC_BASE_OFFSET 5
uint8_t dataCharacteristicValue[MAX_CHARACTERISTIC_VALUE_LENGTH];
uint8_t configurationCharacteristicValue[sizeof(uint8_t)*3];
uint8_t batteryLevelCharacteristicValue[sizeof(float)];

uint16_t batteryLevel = 69;

#define SERVICE_UUID "ca51b65e-1c92-4e54-9bd7-fc1088f48832"
#define CONFIGURATION_CHARACTERISTIC_UUID "816ad53c-29df-4699-b25a-4acdf89699d6"
#define DATA_CHARACTERISTIC_UUID "bb52dc35-1a47-41c1-ae97-ce138dbf2cab"
#define BATTERY_SERVICE_UUID BLEUUID((uint16_t)0x180F)

BLEServer *pServer;

BLEService *pService;
BLECharacteristic *pConfigurationCharacteristic;
BLECharacteristic *pDataCharacteristic;

BLEService *pBatteryService;
BLECharacteristic *pBatteryLevelCharacteristic;

void updateConfigurationCharacteristic(boolean notify) {
  configurationCharacteristicValue[0] = configurationBitmask;
  configurationCharacteristicValue[1] = lowByte(dataDelay);
  configurationCharacteristicValue[2] = highByte(dataDelay);
  pConfigurationCharacteristic->setValue((uint8_t *)configurationCharacteristicValue, sizeof(configurationCharacteristicValue));
  if (notify) {
    pConfigurationCharacteristic->notify();
  }
}

void updateBatteryLevelCharacteristic() {
  // FILL
  // pBatteryLevelCharacteristic->setValue(batteryLevel);
}

bool isServerConnected = false;

class MyServerCallbacks: public BLEServerCallbacks {
    void onConnect(BLEServer* pServer) {
      isServerConnected = true;
      Serial.println("connected");

      configurationBitmask = 0;
      dataDelay = 40;

      updateConfigurationCharacteristic(false);
    };

    void onDisconnect(BLEServer* pServer) {
      isServerConnected = false;
      Serial.println("disconnected");

      bno.enterSuspendMode();
      isBnoAwake = false;

      pServer->getAdvertising()->start();
    }
};

class ConfigurationCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) { 
    uint8_t* configurationData = pCharacteristic->getData();

    if (sizeof(configurationData) >= 2) {
      configurationBitmask = configurationData[0];
      uint16_t newDataDelay = (((uint16_t)configurationData[2]) << 8) | ((uint16_t)configurationData[1]);
      if (newDataDelay >= 20) {
        dataDelay = newDataDelay;
      }

      updateConfigurationCharacteristic(true);

      if (configurationBitmask == 0) {
        bno.enterSuspendMode();
        isBnoAwake = false;
      }
      else {
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
  
  pService = pServer->createService(SERVICE_UUID);

  pDataCharacteristic = pService->createCharacteristic(
    DATA_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  BLEDescriptor *pDataDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pDataDescriptor->setValue("Data");
  pDataCharacteristic->addDescriptor(pDataDescriptor);
  pDataCharacteristic->addDescriptor(new BLE2902());

  pConfigurationCharacteristic = pService->createCharacteristic(
    CONFIGURATION_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  pConfigurationCharacteristic->setCallbacks(new ConfigurationCharacteristicCallbacks());
  BLEDescriptor *pConfigurationDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pConfigurationDescriptor->setValue("Configuation");
  pConfigurationCharacteristic->addDescriptor(pConfigurationDescriptor);
  pConfigurationCharacteristic->addDescriptor(new BLE2902());
  
  pService->start();

  pBatteryService = pServer->createService(BATTERY_SERVICE_UUID);
  pBatteryLevelCharacteristic = pBatteryService->createCharacteristic(
    BLEUUID((uint16_t)0x2A19),
    BLECharacteristic::PROPERTY_READ |
    BLECharacteristic::PROPERTY_NOTIFY
  );
  BLEDescriptor *pBatteryLevelDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
  pBatteryLevelDescriptor->setValue("Battery Level");
  pBatteryLevelCharacteristic->addDescriptor(pBatteryLevelDescriptor);
  pBatteryLevelCharacteristic->addDescriptor(new BLE2902());
  pBatteryLevelCharacteristic->setValue(batteryLevel);

  pBatteryService->start();

  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  BLEAdvertisementData *pAdvertisementData = new BLEAdvertisementData();
  pAdvertisementData->setCompleteServices(BLEUUID::fromString(SERVICE_UUID));
  pAdvertisementData->setName(DEVICE_NAME);
  pAdvertisementData->setShortName(DEVICE_NAME);
  pAdvertising->setAdvertisementData(*pAdvertisementData);

  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
}

unsigned long currentTime = 0;
unsigned long lastTimeDataWasUpdated = 0;

int8_t dataBitmask;
uint32_t timestamp;
uint8_t dataCharacterByteOffset;

void sendData() {
  MEMCPY(&dataCharacteristicValue[0], &dataBitmask, 1);
  MEMCPY(&dataCharacteristicValue[1], &timestamp, sizeof(timestamp));
  pDataCharacteristic->setValue((uint8_t *)dataCharacteristicValue, sizeof(dataCharacteristicValue));
  pDataCharacteristic->notify();
  
  memset(&dataCharacteristicValue, 0, sizeof(dataCharacteristicValue));
  dataCharacterByteOffset = DATA_CHARACTERISTIC_BASE_OFFSET;
  dataBitmask = 0;
}

void checkData(uint8_t bitIndex, bool isVector, Adafruit_BNO055::adafruit_vector_type_t vector_type = Adafruit_BNO055::VECTOR_ACCELEROMETER) {
  if (bitRead(configurationBitmask, bitIndex)) {
    uint8_t size = isVector? 6:8;
    if (dataCharacterByteOffset + size > MAX_CHARACTERISTIC_VALUE_LENGTH) {
      sendData();
    }

    bitSet(dataBitmask, bitIndex);

    uint8_t buffer[size];

    if (isVector) {
      bno.getRawVectorData(vector_type, buffer);
    }
    else {
      bno.getRawQuatData(buffer);
    }

    MEMCPY(&dataCharacteristicValue[dataCharacterByteOffset], &buffer, size);
    dataCharacterByteOffset += size;
  }
}

#define BATTERY_LEVEL_DELAY_MS (1000)
unsigned long lastTimeBatteryWasUpdated = 0;


void loop() {
  currentTime = millis();
  if(currentTime >= lastTimeDataWasUpdated + dataDelay) {
    lastTimeDataWasUpdated += dataDelay;

    if (isServerConnected && isBnoAwake && configurationBitmask) {
      memset(dataCharacteristicValue, 0, sizeof(dataCharacteristicValue));

      dataBitmask = 0;
      timestamp = currentTime;
      dataCharacterByteOffset = DATA_CHARACTERISTIC_BASE_OFFSET;

      checkData(ACCELERATION_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_ACCELEROMETER);
      checkData(GRAVITY_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_GRAVITY);
      checkData(LINEAR_ACCELERATION_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_LINEARACCEL);
      checkData(ROTATION_RATE_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_GYROSCOPE);
      checkData(MAGNETOMETER_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_MAGNETOMETER);
      checkData(QUATERNION_CONFIGURATION_BIT_INDEX, false);

      if (dataCharacterByteOffset > DATA_CHARACTERISTIC_BASE_OFFSET) {
        sendData();
      }
    }
  }

  if(currentTime >= lastTimeBatteryWasUpdated + BATTERY_LEVEL_DELAY_MS) {
    lastTimeBatteryWasUpdated += BATTERY_LEVEL_DELAY_MS;
    updateBatteryLevelCharacteristic();
  }
}