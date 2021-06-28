#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <lwipopts.h>

#include <EEPROM.h>

#include <BLE2902.h>
#include <BLEAdvertising.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Arduino.h>

#undef DEFAULT
#include "tensorflow/lite/micro/all_ops_resolver.h"
#include "tensorflow/lite/micro/micro_error_reporter.h"
#include "tensorflow/lite/micro/micro_interpreter.h"
#include "tensorflow/lite/schema/schema_generated.h"
#include "tensorflow/lite/version.h"

Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);

bool isBnoAwake = false;

uint8_t sensorConfigurationBitmask;
uint16_t sensorDataDelay;

#define ACCELERATION_CONFIGURATION_BIT_INDEX 0
#define GRAVITY_CONFIGURATION_BIT_INDEX 1
#define LINEAR_ACCELERATION_CONFIGURATION_BIT_INDEX 2
#define ROTATION_RATE_CONFIGURATION_BIT_INDEX 3
#define MAGNETOMETER_CONFIGURATION_BIT_INDEX 4
#define QUATERNION_CONFIGURATION_BIT_INDEX 5

#define NAME_INDEX 0
#define MAX_NAME_LENGTH 30
#define EEPROM_LENGTH (1 + MAX_NAME_LENGTH)

std::string name = "Ukaton Side Mission";

#define MAX_CHARACTERISTIC_VALUE_LENGTH 23

#define SENSOR_DATA_CHARACTERISTIC_BASE_OFFSET 5
uint8_t sensorDataCharacteristicValue[MAX_CHARACTERISTIC_VALUE_LENGTH];
uint8_t sensorConfigurationCharacteristicValue[sizeof(uint8_t)*3];

uint8_t batteryLevelCharacteristicValue[sizeof(float)];

uint16_t batteryLevel = 69;

#define SERVICE_UUID "ca51b65e-1c92-4e54-9bd7-fc1088f48832"
#define NAME_CHARACTERISTIC_UUID "f655b733-7a02-46d5-ae3a-45947639a772"
#define SENSOR_CONFIGURATION_CHARACTERISTIC_UUID "816ad53c-29df-4699-b25a-4acdf89699d6"
#define SENSOR_DATA_CHARACTERISTIC_UUID "bb52dc35-1a47-41c1-ae97-ce138dbf2cab"
#define BATTERY_SERVICE_UUID BLEUUID((uint16_t)0x180F)

BLEServer *pServer;

BLEService *pService;
BLECharacteristic *pNameCharacteristic;
BLECharacteristic *pSensorConfigurationCharacteristic;
BLECharacteristic *pSensorDataCharacteristic;

BLEService *pBatteryService;
BLECharacteristic *pBatteryLevelCharacteristic;

BLECharacteristic *createCharacteristic(BLEService *pService, const char *uuid, uint32_t properties, const char *name)
{
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(uuid, properties);
    BLEDescriptor *pDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    pDescriptor->setValue(name);
    pCharacteristic->addDescriptor(pDescriptor);
    pCharacteristic->addDescriptor(new BLE2902());
    return pCharacteristic;
}

void updateSensorConfigurationCharacteristic(boolean notify) {
  sensorConfigurationCharacteristicValue[0] = sensorConfigurationBitmask;
  sensorConfigurationCharacteristicValue[1] = lowByte(sensorDataDelay);
  sensorConfigurationCharacteristicValue[2] = highByte(sensorDataDelay);
  pSensorConfigurationCharacteristic->setValue((uint8_t *)sensorConfigurationCharacteristicValue, sizeof(sensorConfigurationCharacteristicValue));
  if (notify) {
    pSensorConfigurationCharacteristic->notify();
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

      sensorConfigurationBitmask = 0;
      sensorDataDelay = 40;

      updateSensorConfigurationCharacteristic(false);
    };

    void onDisconnect(BLEServer* pServer) {
      isServerConnected = false;
      Serial.println("disconnected");

      bno.enterSuspendMode();
      isBnoAwake = false;

      pServer->getAdvertising()->start();
    }
};

class NameCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) { 
    std::string newName = pCharacteristic->getValue();
    name = newName;
    ::esp_ble_gap_set_device_name(name.c_str());
    EEPROM.writeString(1, name.substr(0, min((int) name.length(), MAX_NAME_LENGTH)).c_str());
    EEPROM.commit();
    Serial.print("new name: ");
    Serial.println(name.c_str());
    pCharacteristic->notify();
  }
};

class SensorConfigurationCharacteristicCallbacks: public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) { 
    if (pCharacteristic->m_value.getLength() >= 2) {
      uint8_t* configurationData = pCharacteristic->getData();
      sensorConfigurationBitmask = configurationData[0];
      uint16_t newDataDelay = (((uint16_t)configurationData[2]) << 8) | ((uint16_t)configurationData[1]);
      if (newDataDelay >= 20) {
        sensorDataDelay = newDataDelay;
      }

      updateSensorConfigurationCharacteristic(true);

      if (sensorConfigurationBitmask == 0) {
        bno.enterSuspendMode();
        isBnoAwake = false;
      }
      else {
        if (!isBnoAwake) {
          bno.enterNormalMode();
          isBnoAwake = true;
        }
      }

      pCharacteristic->notify();
    }
  }
};

inline void setupBno() {
  if(!bno.begin()) {
      Serial.println("No BNO055 detected");
  }
  delay(1000);
  bno.setExtCrystalUse(false);
  bno.enterSuspendMode();
}
inline void setupBLE() {
  BLEDevice::init(name);

  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  
  pService = pServer->createService(SERVICE_UUID);

  pNameCharacteristic = createCharacteristic(pService, NAME_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY, "Name");
  pNameCharacteristic->setCallbacks(new NameCharacteristicCallbacks());
  pSensorDataCharacteristic = createCharacteristic(pService, SENSOR_DATA_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "Sensor Data");
  pSensorConfigurationCharacteristic = createCharacteristic(pService, SENSOR_CONFIGURATION_CHARACTERISTIC_UUID, BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE_NR | BLECharacteristic::PROPERTY_NOTIFY, "Sensor Configuration");
  updateSensorConfigurationCharacteristic(false);
  pSensorConfigurationCharacteristic->setCallbacks(new SensorConfigurationCharacteristicCallbacks());
  
  pService->start();

  pBatteryService = pServer->createService(BATTERY_SERVICE_UUID);
  pBatteryLevelCharacteristic = createCharacteristic(pBatteryService, BLEUUID((uint16_t)0x2A19).toString().c_str(), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "Battery Level");
  pBatteryLevelCharacteristic->setValue(batteryLevel);

  pBatteryService->start();
  
  BLEAdvertising *pAdvertising = pServer->getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);

  BLEAdvertisementData *pAdvertisementData = new BLEAdvertisementData();
  pAdvertisementData->setCompleteServices(BLEUUID::fromString(SERVICE_UUID));
  pAdvertisementData->setName(name);
  pAdvertisementData->setShortName(name);
  pAdvertising->setAdvertisementData(*pAdvertisementData);

  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);
  pAdvertising->setMinPreferred(0x12);
  pAdvertising->start();
}

#define EEPROM_SCHEMA 0
inline void setupEEPROM() {
  EEPROM.begin(EEPROM_LENGTH);

  unsigned char schema = EEPROM.readUChar(0);
  if (schema != EEPROM_SCHEMA) {
    // initial schema setup
    schema = EEPROM_SCHEMA;
    EEPROM.writeUChar(0, schema);
    EEPROM.writeString(1, name.substr(0, min((int) name.length(), MAX_NAME_LENGTH)).c_str());
    EEPROM.commit();
  }
  else {
    name = EEPROM.readString(1).c_str();
    ::esp_ble_gap_set_device_name(name.c_str());
    Serial.println(name.c_str());
  }

  Serial.print("name: ");
  Serial.println(name.c_str());

  pNameCharacteristic->setValue(name);
}

void setup() {
  Serial.begin(115200);

  setupBno();
  setupBLE();
  setupEEPROM();
}

unsigned long currentTime = 0;
unsigned long lastTimeDataWasUpdated = 0;

int8_t dataBitmask;
uint32_t timestamp;
uint8_t sensorDataCharacterByteOffset;

void sendSensorData() {
  MEMCPY(&sensorDataCharacteristicValue[0], &dataBitmask, 1);
  MEMCPY(&sensorDataCharacteristicValue[1], &timestamp, sizeof(timestamp));
  pSensorDataCharacteristic->setValue((uint8_t *)sensorDataCharacteristicValue, sizeof(sensorDataCharacteristicValue));
  pSensorDataCharacteristic->notify();
  
  memset(&sensorDataCharacteristicValue, 0, sizeof(sensorDataCharacteristicValue));
  sensorDataCharacterByteOffset = SENSOR_DATA_CHARACTERISTIC_BASE_OFFSET;
  dataBitmask = 0;
}

void checkData(uint8_t bitIndex, bool isVector, Adafruit_BNO055::adafruit_vector_type_t vector_type = Adafruit_BNO055::VECTOR_ACCELEROMETER) {
  if (bitRead(sensorConfigurationBitmask, bitIndex)) {
    uint8_t size = isVector? 6:8;
    if (sensorDataCharacterByteOffset + size > MAX_CHARACTERISTIC_VALUE_LENGTH) {
      sendSensorData();
    }

    bitSet(dataBitmask, bitIndex);

    uint8_t buffer[size];

    if (isVector) {
      bno.getRawVectorData(vector_type, buffer);
    }
    else {
      bno.getRawQuatData(buffer);
    }

    MEMCPY(&sensorDataCharacteristicValue[sensorDataCharacterByteOffset], &buffer, size);
    sensorDataCharacterByteOffset += size;
  }
}

#define BATTERY_LEVEL_DELAY_MS (1000)
unsigned long lastTimeBatteryWasUpdated = 0;

void loop() {
  currentTime = millis();
  if(currentTime >= lastTimeDataWasUpdated + sensorDataDelay) {
    lastTimeDataWasUpdated += sensorDataDelay;

    if (isServerConnected && isBnoAwake && sensorConfigurationBitmask) {
      memset(sensorDataCharacteristicValue, 0, sizeof(sensorDataCharacteristicValue));

      dataBitmask = 0;
      timestamp = currentTime;
      sensorDataCharacterByteOffset = SENSOR_DATA_CHARACTERISTIC_BASE_OFFSET;

      checkData(ACCELERATION_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_ACCELEROMETER);
      checkData(GRAVITY_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_GRAVITY);
      checkData(LINEAR_ACCELERATION_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_LINEARACCEL);
      checkData(ROTATION_RATE_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_GYROSCOPE);
      checkData(MAGNETOMETER_CONFIGURATION_BIT_INDEX, true, Adafruit_BNO055::VECTOR_MAGNETOMETER);
      checkData(QUATERNION_CONFIGURATION_BIT_INDEX, false);

      if (sensorDataCharacterByteOffset > SENSOR_DATA_CHARACTERISTIC_BASE_OFFSET) {
        sendSensorData();
      }
    }
  }

  if(currentTime >= lastTimeBatteryWasUpdated + BATTERY_LEVEL_DELAY_MS) {
    lastTimeBatteryWasUpdated += BATTERY_LEVEL_DELAY_MS;
    updateBatteryLevelCharacteristic();
  }
}