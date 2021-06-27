/* Copyright 2021 Google LLC
Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at
    https://www.apache.org/licenses/LICENSE-2.0
Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

/*
* @author Rikard Lindstrom <rlindsrom@google.com>
* @author Zack Qattan <zack@ukaton.com> (porting to the (ESP32+BNO055)-based Ukaton motion modules using BNO055 motion sensors)
*/

#include <Wire.h>
#include <Adafruit_BNO055.h>
#include <lwipopts.h>

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

#define VERSION 5
#define FLOAT_BYTE_SIZE 4

/************************************************************************
* State
************************************************************************/

enum State
{
    IDLE_DISCONNECTED,          // Arduino was just turned on
    IDLE_CONNECTED,             // BLE was connected
    FILE_TRANSFER,              // File transfer mode
    INFERENCE,                  // Inference is happening and published
    IMU_DATA_PROVIDER,          // Send IMU data over BLE for IMU Trainer
    ERROR_STATE,                // Something went wrong,
    CALIBRATION,                // Calibrate Magnetometer position
    INFERENCE_AND_DATA_PROVIDER // both inference and IMU Data
};

State currentState = IDLE_DISCONNECTED;
State prevState = IDLE_DISCONNECTED;

enum FileTransferType
{
    MODEL_FILE,
    TEST_FILE
};

FileTransferType fileTransferType = MODEL_FILE;

/************************************************************************
* Globals / General
************************************************************************/
bool useMagnetometer = false; // Can be toggled with BLE (disableMagnetometerRx)

/************************************************************************
* BNO055 SENSOR
************************************************************************/
Adafruit_BNO055 bno = Adafruit_BNO055(-1, 0x28);
bool isBnoAwake = false;

void setupIMU()
{
    if (!bno.begin())
    {
        Serial.println("No BNO055 detected");
    }
    delay(1000);
    bno.setExtCrystalUse(false);
    bno.enterSuspendMode();
}

/************************************************************************
* BLE Characteristic / Service UUIDs
************************************************************************/

#define LOCAL_NAME "Ukaton Side Mission"

#define UUID_GEN(val) ("81c30e5c-" val "-4f7d-a886-de3e90749161")

BLEServer *pServer;

bool isServerConnected = false;

// BLEService service(UUID_GEN("0000"));
BLEService *pService;

// BLECharacteristic dataProviderTxChar(UUID_GEN("1001"), BLERead | BLENotify, 9 * FLOAT_BYTE_SIZE);
// BLECharacteristic dataProviderLabelsTxChar(UUID_GEN("1002"), BLERead, 128);
// BLEUnsignedCharCharacteristic versionTxChar(UUID_GEN("1003"), BLERead);
// BLECharacteristic inferenceTxChar(UUID_GEN("1004"), BLERead | BLENotify, 3);
BLECharacteristic *pDataProviderTxChar;
BLECharacteristic *pDataProviderLabelsTxChar;
BLECharacteristic *pVersionTxChar;
BLECharacteristic *pInferenceTxChar;

// BLEUnsignedCharCharacteristic numClassesRxChar(UUID_GEN("2001"), BLEWrite);
// BLEIntCharacteristic numSamplesRxChar(UUID_GEN("2002"), BLEWrite);
// BLEIntCharacteristic captureDelayRxChar(UUID_GEN("2003"), BLEWrite);
// BLEFloatCharacteristic thresholdRxChar(UUID_GEN("2004"), BLEWrite);
// BLEBoolCharacteristic disableMagnetometerRx(UUID_GEN("2005"), BLEWrite);
BLECharacteristic *pNumClassesRxChar;
BLECharacteristic *pNumSamplesRxChar;
BLECharacteristic *pCaptureDelayRxChar;
BLECharacteristic *pThresholdRxChar;
BLECharacteristic *pDisableMagnetometerRxChar;

// BLEUnsignedCharCharacteristic stateRxChar(UUID_GEN("3001"), BLEWrite);
// BLEUnsignedCharCharacteristic stateTxChar(UUID_GEN("3002"), BLERead | BLENotify);
// BLEUnsignedCharCharacteristic fileTransferTypeRxChar(UUID_GEN("3003"), BLEWrite);
// BLEBoolCharacteristic hasModelTxChar(UUID_GEN("3004"), BLERead | BLENotify);
BLECharacteristic *pStateRxChar;
BLECharacteristic *pStateTxChar;
BLECharacteristic *pFileTransferTypeRxChar;
BLECharacteristic *pHasModelTxChar;

// Meta is for future-proofing, we can use it to store and read any 64 bytes
// BLECharacteristic metaRxChar(UUID_GEN("4001"), BLEWrite, 64);
// BLECharacteristic metaTxChar(UUID_GEN("4002"), BLERead, 64);
BLECharacteristic *pMetaRxChar;
BLECharacteristic *pMetaTxChar;

/************************************************************************
* Model file transfer
************************************************************************/

uint8_t *newModelFileData = nullptr;
int newModelFileLength = 0;

// called on inference (gesture detected)
void model_tester_onInference(unsigned char classIndex, unsigned char score, unsigned char velocity)
{
    const byte buffer[]{classIndex, score, velocity};
    //inferenceTxChar.setValue(buffer, 3);
    pInferenceTxChar->setValue((uint8_t *)buffer, 3);
    pInferenceTxChar->notify();
    Serial.print("Inference - class: ");
    Serial.print(classIndex);
    Serial.print(" score: ");
    Serial.println(score);
}

/************************************************************************
* LED / State status functions
************************************************************************/
void setState(State state)
{
    if (state != prevState && state != currentState)
    {
        prevState = currentState;
    }
    currentState = state;
    //stateTxChar.writeValue((unsigned char)state);
    uint8_t stateData[1];
    stateData[0] = state;
    pStateTxChar->setValue((uint8_t *)stateData, 1);
    pStateTxChar->notify();
    switch (currentState)
    {
    case IDLE_DISCONNECTED:
        Serial.println("state is now IDLE_DISCONNECTED");
        break;
    case IDLE_CONNECTED:
        Serial.println("state is now IDLE_CONNECTED");
        break;
    case FILE_TRANSFER:
        Serial.println("state is now FILE_TRANSFER");
        break;
    case INFERENCE:
        Serial.println("state is now INFERENCE");
        break;
    case IMU_DATA_PROVIDER:
        Serial.println("state is now IMU_DATA_PROVIDER");
        break;
    case ERROR_STATE:
        Serial.println("state is now ERROR_STATE");
        break;
    case CALIBRATION:
        //data_provider::calibrate();
        Serial.println("state is now CALIBRATION");
        break;
    case INFERENCE_AND_DATA_PROVIDER:
        Serial.println("state is now INFERENCE_AND_DATA_PROVIDER");
        break;
    default:
        Serial.print("Error: Unknown state: ");
        Serial.println(currentState);
    }
}

/************************************************************************
* model_tester
************************************************************************/

namespace model_tester
{
    /************************************************************************
* Capture settings variables
************************************************************************/

    int numSamples = 10;
    int samplesRead = numSamples;
    float accelerationThreshold = 0.167;
    int captureDelay = 125;
    unsigned char numClasses = 3;
    bool disableMagnetometer = false;

    float maxVelocity = 0.;
    int lastCaptureTimestamp = 0;

    /************************************************************************
* Model loading / inference variables
************************************************************************/

    bool isModelLoaded = false;
    uint8_t interpreterBuffer[sizeof(tflite::MicroInterpreter)];

    constexpr int tensorArenaSize = 8 * 1024;
    alignas(16) byte tensorArena[tensorArenaSize];

    // global variables used for TensorFlow Lite (Micro)
    tflite::MicroErrorReporter tflErrorReporter;

    // pull in all the TFLM ops, you can remove this line and
    // only pull in the TFLM ops you need, if would like to reduce
    // the compiled size of the sketch.
    tflite::AllOpsResolver tflOpsResolver;

    const tflite::Model *tflModel = nullptr;
    tflite::MicroInterpreter *tflInterpreter = nullptr;
    TfLiteTensor *tflInputTensor = nullptr;
    TfLiteTensor *tflOutputTensor = nullptr;

    /************************************************************************
* Setters
************************************************************************/

    void setCaptureDelay(int val)
    {
        captureDelay = val;
    }

    void setNumSamples(int val)
    {
        numSamples = val;
        samplesRead = val;
    }

    void setThreshold(float val)
    {
        accelerationThreshold = val;
    }

    void setNumClasses(unsigned char val)
    {
        numClasses = val;
    }

    void setDisableMagnetometer(bool val)
    {
        disableMagnetometer = val;
    }

    /************************************************************************
* Model loading
************************************************************************/

    void loadModel(unsigned char model[])
    {
        // get the TFL representation of the model byte array
        tflModel = tflite::GetModel(model);
        if (tflModel->version() != TFLITE_SCHEMA_VERSION)
        {
            Serial.println("Model schema mismatch!");
            while (1)
                ;
        }

        // Create an interpreter to run the model
        tflInterpreter = new (interpreterBuffer) tflite::MicroInterpreter(tflModel, tflOpsResolver, tensorArena, tensorArenaSize, &tflErrorReporter);

        // Allocate memory for the model's input and output tensors
        tflInterpreter->AllocateTensors();

        // Get pointers for the model's input and output tensors
        tflInputTensor = tflInterpreter->input(0);
        tflOutputTensor = tflInterpreter->output(0);

        isModelLoaded = true;
    }

    /************************************************************************
* Life cycle
************************************************************************/

    // uncomment to log out each capture as CSV
    //#define DEBUG

    void update(float buffer[])
    {

        int now = millis();

        if (samplesRead == numSamples)
        {

            // Honor captureDelay setting
            if (now - lastCaptureTimestamp < captureDelay)
            {
                return;
            }

            const float aSum = (fabs(buffer[0]) + fabs(buffer[1]) + fabs(buffer[2]) + fabs(buffer[3]) + fabs(buffer[4]) + fabs(buffer[5])) / 6.0;

            // check if it's above the threshold
            if (aSum >= accelerationThreshold)
            {
#ifdef DEBUG
                Serial.println("Capture started:");
                Serial.println("ax,ay,az,gx,gy,gz,mx,my,mz");
#endif

                // reset the sample read count
                samplesRead = 0;
                maxVelocity = 0.;
            }
        }

        if (samplesRead < numSamples)
        {
            const int dataLen = disableMagnetometer ? 6 : 9;

#ifdef DEBUG
            for (int j = 0; j < dataLen; j++)
            {
                Serial.print(buffer[j], 16);
                if (j < dataLen - 1)
                {
                    Serial.print(",");
                }
            }
            Serial.println();
#endif

            const float velocity = (fabs(buffer[0]) + fabs(buffer[1]) + fabs(buffer[2])) / 3.0;
            maxVelocity = max(maxVelocity, velocity);

            tflInputTensor->data.f[samplesRead * dataLen + 0] = buffer[0];
            tflInputTensor->data.f[samplesRead * dataLen + 1] = buffer[1];
            tflInputTensor->data.f[samplesRead * dataLen + 2] = buffer[2];
            tflInputTensor->data.f[samplesRead * dataLen + 3] = buffer[3];
            tflInputTensor->data.f[samplesRead * dataLen + 4] = buffer[4];
            tflInputTensor->data.f[samplesRead * dataLen + 5] = buffer[5];
            if (!disableMagnetometer)
            {
                tflInputTensor->data.f[samplesRead * dataLen + 6] = buffer[6];
                tflInputTensor->data.f[samplesRead * dataLen + 7] = buffer[7];
                tflInputTensor->data.f[samplesRead * dataLen + 8] = buffer[8];
            }

            samplesRead++;

            if (samplesRead == numSamples)
            {
                // Run inferencing
                TfLiteStatus invokeStatus = tflInterpreter->Invoke();
                if (invokeStatus != kTfLiteOk)
                {
                    Serial.println("Invoke failed!");
                    while (1)
                        ;
                    return;
                }
#ifdef DEBUG
                Serial.println();
                Serial.println("-----------------------------------------");
#endif

                // Loop through the output tensor values from the model
                unsigned char maxIndex = 0;
                float maxValue = 0;
                for (int i = 0; i < numClasses; i++)
                {
                    float _value = tflOutputTensor->data.f[i];
                    Serial.print("class: ");
                    Serial.println(i);
                    Serial.print(" score: ");
                    Serial.println(_value, 6);

                    if (_value > maxValue)
                    {
                        maxValue = _value;
                        maxIndex = i;
                    }
                }

                // callback
                unsigned char velocity = (unsigned char)(((maxVelocity - accelerationThreshold) / (1.0 - accelerationThreshold)) * 255.999);
                unsigned char score = (unsigned char)(maxValue * 255.999);

                model_tester_onInference(maxIndex, score, velocity);

                Serial.print("Winner: ");
                Serial.println(maxIndex);

                // timestamp to follow the capture delay setting
                lastCaptureTimestamp = now;
            }
        }
    }

    void runTest(float *data, int len)
    {
        const int numTests = len / numSamples;
        const int dataLen = disableMagnetometer ? 6 : 9;
        samplesRead = numSamples;
        const int tmpCaptureDelay = captureDelay;
        captureDelay = 0;
        for (int i = 0; i < len; i += dataLen)
        {
            if (disableMagnetometer)
            {
                float buffer[] = {
                    data[i + 0],
                    data[i + 1],
                    data[i + 2],
                    data[i + 3],
                    data[i + 4],
                    data[i + 5]};
                update(buffer);
            }
            else
            {
                float buffer[] = {
                    data[i + 0],
                    data[i + 1],
                    data[i + 2],
                    data[i + 3],
                    data[i + 4],
                    data[i + 5],
                    data[i + 6],
                    data[i + 7],
                    data[i + 8]};

                update(buffer);
            }
        }

        captureDelay = tmpCaptureDelay;
    }
}

/************************************************************************
* Callbacks
************************************************************************/

// called when calibration completes
void data_provider_calibrationComplete()
{
    setState(prevState);
}

// called on file transfer complete
void onBLEFileReceived(uint8_t *file_data, int file_length)
{
    switch (fileTransferType)
    {
    case MODEL_FILE:
        // Queue up the model swap
        newModelFileData = file_data;
        newModelFileLength = file_length;
        break;
    case TEST_FILE:
    {
        int floatLength = file_length / 4;
        float buffer[floatLength];
        for (int i = 0; i < file_length; i += 4)
        {
            union u_tag
            {
                byte b[4];
                float fval;
            } u;

            u.b[0] = file_data[i + 0];
            u.b[1] = file_data[i + 1];
            u.b[2] = file_data[i + 2];
            u.b[3] = file_data[i + 3];

            buffer[i / 4] = u.fval;
        }
        // set state to inference so we can capture result
        setState(INFERENCE);
        model_tester::runTest(buffer, floatLength);
    }
    break;
    default:
        Serial.println("Error: unkown file type");
        setState(ERROR_STATE);
    }
}

/************************************************************************
* BLE Event handlers
************************************************************************/

class MyServerCallbacks : public BLEServerCallbacks
{
    void onConnect(BLEServer *pServer)
    {
        isServerConnected = true;
        Serial.println("connected");

        if (!isBnoAwake) {
          bno.enterNormalMode();
          isBnoAwake = true;
        }
    };

    void onDisconnect(BLEServer *pServer)
    {
        isServerConnected = false;
        Serial.println("disconnected");
        setState(IDLE_DISCONNECTED);

        bno.enterSuspendMode();
        isBnoAwake = false;

        pServer->getAdvertising()->start();
    }
};

/*
void handleNumSamplesRxWritten(BLEDevice central, BLECharacteristic characteristic)
{
  model_tester::setNumSamples(numSamplesRxChar.value());
  Serial.print("Received numSamples: ");
  Serial.println(numSamplesRxChar.value());
}
*/
class NumSamplesRxCharCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        uint8_t *data = pCharacteristic->getData();
        int numSamples = data[0];
        model_tester::setNumSamples(numSamples);
        Serial.print("Received numSamples:");
        Serial.println(numSamples);
    }
};

/*
void handleThresholdRxWritten(BLEDevice central, BLECharacteristic characteristic)
{
  model_tester::setThreshold(thresholdRxChar.value());
  Serial.print("Received threshold: ");
  Serial.println(thresholdRxChar.value(), 4);
}
*/
class ThresholdRxCharCharCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        union
        {
            float threshold;
            uint8_t raw[4];
        } temp;

        uint8_t *data = pCharacteristic->getData();
        temp.raw[0] = data[0];
        temp.raw[1] = data[1];
        temp.raw[2] = data[2];
        temp.raw[3] = data[3];
        model_tester::setThreshold(temp.threshold);
        Serial.print("Received threshold:");
        Serial.println(temp.threshold);
    }
};

/*
void handleCaptureDelayRxWritten(BLEDevice central, BLECharacteristic characteristic)
{
  model_tester::setCaptureDelay(captureDelayRxChar.value());
  Serial.print("Received delay: ");
  Serial.println(captureDelayRxChar.value());
}
*/
class CaptureDelayRxCharCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        //model_tester::setCaptureDelay(captureDelayRxChar.value());
        uint8_t *data = pCharacteristic->getData();
        int captureDelay = data[0];
        model_tester::setCaptureDelay(captureDelay);
        Serial.print("Received delay:");
        Serial.println(captureDelay);
    }
};

/*
void handleNumClassesRxWritten(BLEDevice central, BLECharacteristic characteristic)
{
  model_tester::setNumClasses(numClassesRxChar.value());
  Serial.print("Received numClasses: ");
  Serial.println(numClassesRxChar.value());
}
*/
class NumClassesRxCharCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        uint8_t *data = pCharacteristic->getData();
        unsigned char numClasses = data[0];
        model_tester::setNumClasses(numClasses);
        Serial.print("Received numClasses:");
        Serial.println(numClasses);
    }
};

/*
void handleDisableMagnetometerRxWritten(BLEDevice central, BLECharacteristic characteristic)
{
  bool val = disableMagnetometerRx.value();
  model_tester::setDisableMagnetometer(val);

  useMagnetometer = !val;
  
  Serial.print("Received disableMagnetometer: ");
  Serial.println(disableMagnetometerRx.value());
}
*/
class DisableMagnetometerRxCharCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        uint8_t *data = pCharacteristic->getData();
        bool val = (bool)data[0];
        model_tester::setDisableMagnetometer(val);

        useMagnetometer = !val;
        Serial.print("Received disableMagnetometer: ");
        Serial.println(val);
    }
};

/*
void handleStateWritten(BLEDevice central, BLECharacteristic characteristic)
{
  setState((State)stateRxChar.value());
  Serial.print("Received state: ");
  Serial.println(stateRxChar.value());
}
*/
class StateRxCharCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        uint8_t *data = pCharacteristic->getData();
        unsigned char rawState = data[0];
        State state = (State)rawState;
        setState(state);
        Serial.print("Received state:");
        Serial.println(rawState);
    }
};

/*
void handleMetaWritten(BLEDevice central, BLECharacteristic characteristic)
{
  // Meta is just a 64 byte storage for anything, just publish it
  byte values[64];
  metaRxChar.readValue(values, 64);
  metaTxChar.writeValue(values, 64);
}
*/
class MetaRxCharCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        // Meta is just a 64 byte storage for anything, just publish it
        //byte values[64];
        uint8_t *data = pCharacteristic->getData();
        //pMetaRxChar.readValue(values, 64);
        //pMetaRxChar.writeValue(values, 64);
        Serial.print("Received Metadata:");
        Serial.println(pCharacteristic->getValue().c_str());
        pMetaTxChar->setValue(data, 64);
        pMetaTxChar->notify();
    }
};

/*
void handleFileTransferTypeWritten(BLEDevice central, BLECharacteristic characteristic)
{
  fileTransferType = (FileTransferType)fileTransferTypeRxChar.value();
  Serial.print("Received fileTransferType: ");
  Serial.println(fileTransferType);
}
*/
class FileTransferTypeRxCharCallbacks : public BLECharacteristicCallbacks
{
    void onWrite(BLECharacteristic *pCharacteristic)
    {
        //fileTransferType = (FileTransferType)pFileTransferTypeRxChar.value();
        uint8_t *data = pCharacteristic->getData();
        unsigned char rawFileTransferType = data[0];
        fileTransferType = (FileTransferType)rawFileTransferType;
        Serial.print("Received fileTransferType: ");
        Serial.println(rawFileTransferType);
    }
};

/************************************************************************
* Create BLE Characteristic
************************************************************************/

BLECharacteristic *createCharacteristic(const char *uuid, uint32_t properties, const char *name, int size = 1)
{
    BLECharacteristic *pCharacteristic = pService->createCharacteristic(uuid, properties);
    uint8_t pCharacteristicData[size];
    memset(pCharacteristicData, 0, size);
    pCharacteristic->setValue((uint8_t *)pCharacteristicData, size);
    BLEDescriptor *pDescriptor = new BLEDescriptor(BLEUUID((uint16_t)0x2901));
    pDescriptor->setValue(name);
    pCharacteristic->addDescriptor(pDescriptor);
    pCharacteristic->addDescriptor(new BLE2902());
    return pCharacteristic;
}

/************************************************************************
* ble_file_transfer
************************************************************************/

namespace ble_file_transfer
{
    // Controls how large a file the board can receive. We double-buffer the files
    // as they come in, so you'll need twice this amount of RAM. The default is set
    // to 50KB.
    constexpr int32_t file_maximum_byte_count = (25 * 1024);

// Macro based on a master UUID that can be modified for each characteristic.
#define FILE_TRANSFER_UUID(val) ("bf88b656-" val "-4a61-86e0-769c741026c0")

    // How big each transfer block can be. In theory this could be up to 512 bytes, but
    // in practice I've found that going over 128 affects reliability of the connection.
    constexpr int32_t file_block_byte_count = 128;

    // Where each data block is written to during the transfer.
    //BLECharacteristic file_block_characteristic(FILE_TRANSFER_UUID("3000"), BLEWrite, file_block_byte_count);
    BLECharacteristic *pFile_block_characteristic;

    // Write the expected total length of the file in bytes to this characteristic
    // before sending the command to transfer a file.
    //BLECharacteristic file_length_characteristic(FILE_TRANSFER_UUID("3001"), BLERead | BLEWrite, sizeof(uint32_t));
    BLECharacteristic *pFile_length_characteristic;

    // Read-only attribute that defines how large a file the sketch can handle.
    //BLECharacteristic file_maximum_length_characteristic(FILE_TRANSFER_UUID("3002"), BLERead, sizeof(uint32_t));
    BLECharacteristic *pFile_maximum_length_characteristic;

    // Write the checksum that you expect for the file here before you trigger the transfer.
    //BLECharacteristic file_checksum_characteristic(FILE_TRANSFER_UUID("3003"), BLERead | BLEWrite, sizeof(uint32_t));
    BLECharacteristic *pFile_checksum_characteristic;

    // Writing a command of 1 starts a file transfer (the length and checksum characteristics should already have been set).
    // A command of 2 tries to cancel any pending file transfers. All other commands are undefined.
    //BLECharacteristic command_characteristic(FILE_TRANSFER_UUID("3004"), BLEWrite, sizeof(uint32_t));
    BLECharacteristic *pCommand_characteristic;

    // A status set to 0 means a file transfer succeeded, 1 means there was an error, and 2 means a file transfer is
    // in progress.
    //BLECharacteristic transfer_status_characteristic(FILE_TRANSFER_UUID("3005"), BLERead | BLENotify, sizeof(uint32_t));
    BLECharacteristic *pTransfer_status_characteristic;

    // Informative text describing the most recent error, for user interface purposes.
    constexpr int32_t error_message_byte_count = 128;
    //BLECharacteristic error_message_characteristic(FILE_TRANSFER_UUID("3006"), BLERead | BLENotify, error_message_byte_count);
    BLECharacteristic *pError_message_characteristic;

    // Internal globals used for transferring the file.
    //uint8_t file_buffers[2][file_maximum_byte_count];
    uint8_t *file_buffers[2];
    int finished_file_buffer_index = -1;
    uint8_t *finished_file_buffer = nullptr;
    int32_t finished_file_buffer_byte_count = 0;

    uint8_t *in_progress_file_buffer = nullptr;
    int32_t in_progress_bytes_received = 0;
    int32_t in_progress_bytes_expected = 0;
    uint32_t in_progress_checksum = 0;

    void notifyError(const String &error_message)
    {
        Serial.println(error_message);
        constexpr int32_t error_status_code = 1;
        //transfer_status_characteristic.writeValue(error_status_code);
        int32_t error_status_code_buffer[1];
        error_status_code_buffer[0] = error_status_code;
        pTransfer_status_characteristic->setValue((uint8_t *)error_status_code_buffer, sizeof(int32_t));
        pTransfer_status_characteristic->notify();

        const char *error_message_bytes = error_message.c_str();
        uint8_t error_message_buffer[error_message_byte_count];
        bool at_string_end = false;
        for (int i = 0; i < error_message_byte_count; ++i)
        {
            const bool at_last_byte = (i == (error_message_byte_count - 1));
            if (!at_string_end && !at_last_byte)
            {
                const char current_char = error_message_bytes[i];
                if (current_char == 0)
                {
                    at_string_end = true;
                }
                else
                {
                    error_message_buffer[i] = current_char;
                }
            }

            if (at_string_end || at_last_byte)
            {
                error_message_buffer[i] = 0;
            }
        }
        //error_message_characteristic.writeValue(error_message_buffer, error_message_byte_count);
        pError_message_characteristic->setValue((uint8_t *)error_message_buffer, error_message_byte_count);
        pError_message_characteristic->notify();
    }

    void notifySuccess()
    {
        constexpr int32_t success_status_code = 0;
        //transfer_status_characteristic.writeValue(success_status_code);
        int32_t success_status_code_buffer[1];
        success_status_code_buffer[0] = success_status_code;
        pTransfer_status_characteristic->setValue((uint8_t *)success_status_code_buffer, sizeof(int32_t));
        pTransfer_status_characteristic->notify();
    }

    void notifyInProgress()
    {
        constexpr int32_t in_progress_status_code = 2;
        //transfer_status_characteristic.writeValue(in_progress_status_code);
        int32_t in_progress_status_code_buffer[1];
        in_progress_status_code_buffer[0] = in_progress_status_code;
        pTransfer_status_characteristic->setValue((uint8_t *)in_progress_status_code_buffer, sizeof(int32_t));
        pTransfer_status_characteristic->notify();
    }

    // See http://home.thep.lu.se/~bjorn/crc/ for more information on simple CRC32 calculations.
    uint32_t crc32_for_byte(uint32_t r)
    {
        for (int j = 0; j < 8; ++j)
        {
            r = (r & 1 ? 0 : (uint32_t)0xedb88320L) ^ r >> 1;
        }
        return r ^ (uint32_t)0xff000000L;
    }

    uint32_t crc32(const uint8_t *data, size_t data_length)
    {
        constexpr int table_size = 256;
        static uint32_t table[table_size];
        static bool is_table_initialized = false;
        if (!is_table_initialized)
        {
            for (size_t i = 0; i < table_size; ++i)
            {
                table[i] = crc32_for_byte(i);
            }
            is_table_initialized = true;
        }
        uint32_t crc = 0;
        for (size_t i = 0; i < data_length; ++i)
        {
            const uint8_t crc_low_byte = static_cast<uint8_t>(crc);
            const uint8_t data_byte = data[i];
            const uint8_t table_index = crc_low_byte ^ data_byte;
            crc = table[table_index] ^ (crc >> 8);
        }
        return crc;
    }

    // This is a small test function for the CRC32 implementation, not normally called but left in
    // for debugging purposes. We know the expected CRC32 of [97, 98, 99, 100, 101] is 2240272485,
    // or 0x8587d865, so if anything else is output we know there's an error in the implementation.
    void testCrc32()
    {
        constexpr int test_array_length = 5;
        const uint8_t test_array[test_array_length] = {97, 98, 99, 100, 101};
        const uint32_t test_array_crc32 = crc32(test_array, test_array_length);
        Serial.println(String("CRC32 for [97, 98, 99, 100, 101] is 0x") + String(test_array_crc32, 16) +
                       String(" (") + String(test_array_crc32) + String(")"));
    }

    void onFileTransferComplete()
    {
        uint32_t computed_checksum = crc32(in_progress_file_buffer, in_progress_bytes_expected);
        ;
        if (in_progress_checksum != computed_checksum)
        {
            notifyError(String("File transfer failed: Expected checksum 0x") + String(in_progress_checksum, 16) +
                        String(" but received 0x") + String(computed_checksum, 16));
            in_progress_file_buffer = nullptr;
            return;
        }

        if (finished_file_buffer_index == 0)
        {
            finished_file_buffer_index = 1;
        }
        else
        {
            finished_file_buffer_index = 0;
        }
        finished_file_buffer = &file_buffers[finished_file_buffer_index][0];
        ;
        finished_file_buffer_byte_count = in_progress_bytes_expected;

        in_progress_file_buffer = nullptr;
        in_progress_bytes_received = 0;
        in_progress_bytes_expected = 0;

        notifySuccess();

        onBLEFileReceived(finished_file_buffer, finished_file_buffer_byte_count);
    }

    /*
    void onFileBlockWritten(BLEDevice central, BLECharacteristic characteristic)
    {
        if (in_progress_file_buffer == nullptr)
        {
            notifyError("File block sent while no valid command is active");
            return;
        }

        const int32_t file_block_length = characteristic.valueLength();
        if (file_block_length > file_block_byte_count)
        {
            notifyError(String("Too many bytes in block: Expected ") + String(file_block_byte_count) +
                        String(" but received ") + String(file_block_length));
            in_progress_file_buffer = nullptr;
            return;
        }

        const int32_t bytes_received_after_block = in_progress_bytes_received + file_block_length;
        if ((bytes_received_after_block > in_progress_bytes_expected) ||
            (bytes_received_after_block > file_maximum_byte_count))
        {
            notifyError(String("Too many bytes: Expected ") + String(in_progress_bytes_expected) +
                        String(" but received ") + String(bytes_received_after_block));
            in_progress_file_buffer = nullptr;
            return;
        }

        uint8_t *file_block_buffer = in_progress_file_buffer + in_progress_bytes_received;
        characteristic.readValue(file_block_buffer, file_block_length);

// Enable this macro to show the data in the serial log.
#ifdef ENABLE_LOGGING
        Serial.print("Data received: length = ");
        Serial.println(file_block_length);

        char string_buffer[file_block_byte_count + 1];
        for (int i = 0; i < file_block_byte_count; ++i)
        {
            unsigned char value = file_block_buffer[i];
            if (i < file_block_length)
            {
                string_buffer[i] = value;
            }
            else
            {
                string_buffer[i] = 0;
            }
        }
        string_buffer[file_block_byte_count] = 0;
        Serial.println(String(string_buffer));
#endif // ENABLE_LOGGING

        if (bytes_received_after_block == in_progress_bytes_expected)
        {
            onFileTransferComplete();
        }
        else
        {
            in_progress_bytes_received = bytes_received_after_block;
        }
    }
    */
    class File_block_characteristic_callbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            if (in_progress_file_buffer == nullptr)
            {
                notifyError("File block sent while no valid command is active");
                return;
            }

            uint8_t *data = pCharacteristic->getData();
            size_t size = pCharacteristic->m_value.getLength();

            //const int32_t file_block_length = characteristic.valueLength();
            const int32_t file_block_length = size;
            if (file_block_length > file_block_byte_count)
            {
                notifyError(String("Too many bytes in block: Expected ") + String(file_block_byte_count) +
                            String(" but received ") + String(file_block_length));
                in_progress_file_buffer = nullptr;
                return;
            }

            const int32_t bytes_received_after_block = in_progress_bytes_received + file_block_length;
            if ((bytes_received_after_block > in_progress_bytes_expected) ||
                (bytes_received_after_block > file_maximum_byte_count))
            {
                notifyError(String("Too many bytes: Expected ") + String(in_progress_bytes_expected) +
                            String(" but received ") + String(bytes_received_after_block));
                in_progress_file_buffer = nullptr;
                return;
            }

            uint8_t *file_block_buffer = in_progress_file_buffer + in_progress_bytes_received;
            //characteristic.readValue(file_block_buffer, file_block_length);
            MEMCPY(file_block_buffer, data, file_block_length);

// Enable this macro to show the data in the serial log.
//#define ENABLE_LOGGING
#ifdef ENABLE_LOGGING
            Serial.print("Data received: length = ");
            Serial.println(file_block_length);

            char string_buffer[file_block_byte_count + 1];
            for (int i = 0; i < file_block_byte_count; ++i)
            {
                unsigned char value = file_block_buffer[i];
                if (i < file_block_length)
                {
                    string_buffer[i] = value;
                }
                else
                {
                    string_buffer[i] = 0;
                }
            }
            string_buffer[file_block_byte_count] = 0;
            Serial.println(String(string_buffer));
#endif // ENABLE_LOGGING

            if (bytes_received_after_block == in_progress_bytes_expected)
            {
                onFileTransferComplete();
            }
            else
            {
                in_progress_bytes_received = bytes_received_after_block;
            }
        }
    };

    void startFileTransfer()
    {

        if (in_progress_file_buffer != nullptr)
        {
            notifyError("File transfer command received while previous transfer is still in progress");
            return;
        }

        //file_length_characteristic.readValue(file_length_value);
        uint8_t *file_length_data = pFile_length_characteristic->getData();
        int32_t file_length_value = ((int32_t)file_length_data[0]) | (((int32_t)file_length_data[1])) << 8 | ((int32_t)file_length_data[2]) << 16 | ((int32_t)file_length_data[3]) << 24;
        if (file_length_value > file_maximum_byte_count)
        {
            notifyError(
                String("File too large: Maximum is ") + String(file_maximum_byte_count) +
                String(" bytes but request is ") + String(file_length_value) + String(" bytes"));
            return;
        }

        //file_checksum_characteristic.readValue(in_progress_checksum);
        uint8_t *in_progress_checksum_data = pFile_checksum_characteristic->getData();
        in_progress_checksum = ((int32_t)in_progress_checksum_data[0]) | (((int32_t)in_progress_checksum_data[1])) << 8 | ((int32_t)in_progress_checksum_data[2]) << 16 | ((int32_t)in_progress_checksum_data[3]) << 24;

        int in_progress_file_buffer_index;
        if (finished_file_buffer_index == 0)
        {
            in_progress_file_buffer_index = 1;
        }
        else
        {
            in_progress_file_buffer_index = 0;
        }

        in_progress_file_buffer = &file_buffers[in_progress_file_buffer_index][0];
        in_progress_bytes_received = 0;
        in_progress_bytes_expected = file_length_value;

        notifyInProgress();
    }

    void cancelFileTransfer()
    {
        if (in_progress_file_buffer != nullptr)
        {
            notifyError("File transfer cancelled");
            in_progress_file_buffer = nullptr;
        }
    }

    /*
    void onCommandWritten(BLEDevice central, BLECharacteristic characteristic)
    {
        int32_t command_value;
        characteristic.readValue(command_value);

        if ((command_value != 1) && (command_value != 2))
        {
            notifyError(String("Bad command value: Expected 1 or 2 but received ") + String(command_value));
            return;
        }

        if (command_value == 1)
        {
            startFileTransfer();
        }
        else if (command_value == 2)
        {
            cancelFileTransfer();
        }
    }
    */

    class Command_characteristic_callbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            int32_t command_value;
            //characteristic.readValue(command_value);
            uint8_t *command_value_data = pCharacteristic->getData();
            command_value = ((int32_t)command_value_data[0]) | (((int32_t)command_value_data[1])) << 8 | ((int32_t)command_value_data[2]) << 16 | ((int32_t)command_value_data[3]) << 24;

            if ((command_value != 1) && (command_value != 2))
            {
                notifyError(String("Bad command value: Expected 1 or 2 but received ") + String(command_value));
                return;
            }

            if (command_value == 1)
            {
                startFileTransfer();
            }
            else if (command_value == 2)
            {
                cancelFileTransfer();
            }
        }
    };

    // Starts the BLE handling you need to support the file transfer.
    void setupBLEFileTransfer()
    {
        file_buffers[0] = (uint8_t *)malloc(file_maximum_byte_count * sizeof(uint8_t));
        file_buffers[1] = (uint8_t *)malloc(file_maximum_byte_count * sizeof(uint8_t));

        // Add in the characteristics we'll be making available.
        //file_block_characteristic.setEventHandler(BLEWritten, onFileBlockWritten);
        //service.addCharacteristic(file_block_characteristic);
        pFile_block_characteristic = createCharacteristic(FILE_TRANSFER_UUID("3000"), BLECharacteristic::PROPERTY_WRITE, "file_block_characteristic", file_block_byte_count);
        pFile_block_characteristic->setCallbacks(new File_block_characteristic_callbacks());

        //service.addCharacteristic(file_length_characteristic);
        pFile_length_characteristic = createCharacteristic(FILE_TRANSFER_UUID("3001"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "file_length_characteristic", sizeof(uint32_t));

        //file_maximum_length_characteristic.writeValue(file_maximum_byte_count);
        //service.addCharacteristic(file_maximum_length_characteristic);
        pFile_maximum_length_characteristic = createCharacteristic(FILE_TRANSFER_UUID("3002"), BLECharacteristic::PROPERTY_READ, "file_maximum_length_characteristic", sizeof(uint32_t));
        int32_t pFile_maximum_length_characteristicData[1];
        pFile_maximum_length_characteristicData[0] = file_maximum_byte_count;
        pFile_maximum_length_characteristic->setValue((uint8_t *)pFile_maximum_length_characteristicData, sizeof(int32_t));

        //service.addCharacteristic(file_checksum_characteristic);
        pFile_checksum_characteristic = createCharacteristic(FILE_TRANSFER_UUID("3003"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE, "file_checksum_characteristic", sizeof(uint32_t));

        //command_characteristic.setEventHandler(BLEWritten, onCommandWritten);
        //service.addCharacteristic(command_characteristic);
        pCommand_characteristic = createCharacteristic(FILE_TRANSFER_UUID("3004"), BLECharacteristic::PROPERTY_WRITE, "command_characteristic", sizeof(uint32_t));
        pCommand_characteristic->setCallbacks(new Command_characteristic_callbacks());

        //service.addCharacteristic(transfer_status_characteristic);
        pTransfer_status_characteristic = createCharacteristic(FILE_TRANSFER_UUID("3005"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "transfer_status_characteristic", sizeof(uint32_t));
        //service.addCharacteristic(error_message_characteristic);
        pError_message_characteristic = createCharacteristic(FILE_TRANSFER_UUID("3006"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "error_message_characteristic", error_message_byte_count);
    }

    // Called in your loop function to handle BLE housekeeping.
    void updateBLEFileTransfer()
    {
        /*
        BLEDevice central = BLE.central();
        static bool was_connected_last = false;
        if (central && !was_connected_last)
        {
            Serial.print("Connected to central: ");
            Serial.println(central.address());
        }
        was_connected_last = central;
        */
    }

    bool isTransfering()
    {
        return in_progress_file_buffer != nullptr;
    }
}

/************************************************************************
* Main / Lifecycle
************************************************************************/

void setupBLE()
{
    BLEDevice::init(LOCAL_NAME);

    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    pService = pServer->createService(BLEUUID(UUID_GEN("0000")), 200);

    pDataProviderTxChar = createCharacteristic(UUID_GEN("1001"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "dataProviderTxChar", 9 * FLOAT_BYTE_SIZE);
    pDataProviderLabelsTxChar = createCharacteristic(UUID_GEN("1002"), BLECharacteristic::PROPERTY_READ, "dataProviderLabelsTxChar", 128);
    pVersionTxChar = createCharacteristic(UUID_GEN("1003"), BLECharacteristic::PROPERTY_READ, "versionTxChar");
    pInferenceTxChar = createCharacteristic(UUID_GEN("1004"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "inferenceTxChar", 3);

    pNumClassesRxChar = createCharacteristic(UUID_GEN("2001"), BLECharacteristic::PROPERTY_WRITE, "numClassesRxChar");
    pNumSamplesRxChar = createCharacteristic(UUID_GEN("2002"), BLECharacteristic::PROPERTY_WRITE, "numSamplesRxChar");
    pCaptureDelayRxChar = createCharacteristic(UUID_GEN("2003"), BLECharacteristic::PROPERTY_WRITE, "captureDelayRxChar");
    pThresholdRxChar = createCharacteristic(UUID_GEN("2004"), BLECharacteristic::PROPERTY_WRITE, "thresholdRxChar");
    pDisableMagnetometerRxChar = createCharacteristic(UUID_GEN("2005"), BLECharacteristic::PROPERTY_WRITE, "disableMagnetometerRx");

    pStateRxChar = createCharacteristic(UUID_GEN("3001"), BLECharacteristic::PROPERTY_WRITE, "stateRxChar");
    pStateTxChar = createCharacteristic(UUID_GEN("3002"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "stateTxChar");
    pFileTransferTypeRxChar = createCharacteristic(UUID_GEN("3003"), BLECharacteristic::PROPERTY_WRITE, "fileTransferTypeRxChar");
    pHasModelTxChar = createCharacteristic(UUID_GEN("3004"), BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY, "hasModelTxChar");

    pMetaRxChar = createCharacteristic(UUID_GEN("4001"), BLECharacteristic::PROPERTY_WRITE, "metaRxChar", 64);
    pMetaTxChar = createCharacteristic(UUID_GEN("4002"), BLECharacteristic::PROPERTY_READ, "metaTxChar", 64);

    // Event driven reads.
    // numClassesRxChar.setEventHandler(BLEWritten, handleNumClassesRxWritten);
    // numSamplesRxChar.setEventHandler(BLEWritten, handleNumSamplesRxWritten);
    // thresholdRxChar.setEventHandler(BLEWritten, handleThresholdRxWritten);
    // captureDelayRxChar.setEventHandler(BLEWritten, handleCaptureDelayRxWritten);
    // stateRxChar.setEventHandler(BLEWritten, handleStateWritten);
    // fileTransferTypeRxChar.setEventHandler(BLEWritten, handleFileTransferTypeWritten);
    // metaRxChar.setEventHandler(BLEWritten, handleMetaWritten);
    // disableMagnetometerRx.setEventHandler(BLEWritten, handleDisableMagnetometerRxWritten);
    pNumClassesRxChar->setCallbacks(new NumClassesRxCharCallbacks());
    pNumSamplesRxChar->setCallbacks(new NumSamplesRxCharCallbacks());
    pThresholdRxChar->setCallbacks(new ThresholdRxCharCharCallbacks());
    pCaptureDelayRxChar->setCallbacks(new CaptureDelayRxCharCallbacks());
    pStateRxChar->setCallbacks(new StateRxCharCallbacks());
    pFileTransferTypeRxChar->setCallbacks(new FileTransferTypeRxCharCallbacks());
    pMetaRxChar->setCallbacks(new MetaRxCharCallbacks());
    pDisableMagnetometerRxChar->setCallbacks(new DisableMagnetometerRxCharCallbacks());

    ble_file_transfer::setupBLEFileTransfer();

    pService->start();

    BLEAdvertising *pAdvertising = pServer->getAdvertising();
    pAdvertising->addServiceUUID(pService->getUUID());

    BLEAdvertisementData *pAdvertisementData = new BLEAdvertisementData();
    pAdvertisementData->setCompleteServices(pService->getUUID());
    pAdvertisementData->setName(LOCAL_NAME);
    pAdvertisementData->setShortName(LOCAL_NAME);
    pAdvertising->setAdvertisementData(*pAdvertisementData);

    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    pAdvertising->start();

    // Broadcast sketch version
    //versionTxChar.writeValue(VERSION);
    uint8_t version[1];
    version[0] = VERSION;
    pVersionTxChar->setValue((uint8_t *)version, 1);

    // Used for Tiny Motion Trainer to label / filter values
    //dataProviderLabelsTxChar.writeValue("acc.x, acc.y, acc.z, gyro.x, gyro.y, gyro.z, mag.x, mag.y, max.zl");
    pDataProviderLabelsTxChar->setValue("acc.x, acc.y, acc.z, gyro.x, gyro.y, gyro.z, mag.x, mag.y, max.zl");
}

void setup()
{
    Serial.begin(115200);

    setupIMU();
    setupBLE();
}

inline void updateFileTransfer()
{
    // Update file transfer state
    ble_file_transfer::updateBLEFileTransfer();

    // Check if we should load a new model
    if (newModelFileData != nullptr)
    {
        Serial.println("reloading model");
        model_tester::loadModel(newModelFileData);
        Serial.println("done reloading model");
        newModelFileData = nullptr;

        //hasModelTxChar.writeValue(true);
        bool hasModelTxCharData[1];
        hasModelTxCharData[0] = true;
        pHasModelTxChar->setValue((uint8_t *)hasModelTxCharData, 1);
        pHasModelTxChar->notify();

        // We have a new model, always enter INFERENCE mode
        setState(INFERENCE);
    }
}

inline void updateIMU()
{
    const char bufferSize = useMagnetometer ? 9 : 6;
    float buffer[bufferSize]; // [ax, ay, az, gx, gy, gz, mx, my, mz]

    // Collect the IMU data
    //data_provider::update(buffer, useMagnetometer);
    uint8_t rawBuffer[6];
    int16_t x, y, z;
    x = y = z = 0;

    bno.getRawVectorData(Adafruit_BNO055::VECTOR_ACCELEROMETER, rawBuffer);
    x = ((int16_t)rawBuffer[0]) | (((int16_t)rawBuffer[1]) << 8);
    y = ((int16_t)rawBuffer[2]) | (((int16_t)rawBuffer[3]) << 8);
    z = ((int16_t)rawBuffer[4]) | (((int16_t)rawBuffer[5]) << 8);
    buffer[0] = ((float)x) / 4000.0;
    buffer[1] = ((float)y) / 4000.0;
    buffer[2] = ((float)z) / 4000.0;

    bno.getRawVectorData(Adafruit_BNO055::VECTOR_GYROSCOPE, rawBuffer);
    x = ((int16_t)rawBuffer[0]) | (((int16_t)rawBuffer[1]) << 8);
    y = ((int16_t)rawBuffer[2]) | (((int16_t)rawBuffer[3]) << 8);
    z = ((int16_t)rawBuffer[4]) | (((int16_t)rawBuffer[5]) << 8);
    buffer[3] = ((float)x) / 32000.0;
    buffer[4] = ((float)y) / 32000.0;
    buffer[5] = ((float)z) / 32000.0;

    if (useMagnetometer)
    {
        bno.getRawVectorData(Adafruit_BNO055::VECTOR_MAGNETOMETER, rawBuffer);
        x = ((int16_t)rawBuffer[0]) | (((int16_t)rawBuffer[1]) << 8);
        y = ((int16_t)rawBuffer[2]) | (((int16_t)rawBuffer[3]) << 8);
        z = ((int16_t)rawBuffer[4]) | (((int16_t)rawBuffer[5]) << 8);
        buffer[6] = ((float)x) / 800.0;
        buffer[7] = ((float)y) / 800.0;
        buffer[8] = ((float)z) / 800.0;
    }

    if (currentState == INFERENCE || currentState == INFERENCE_AND_DATA_PROVIDER)
    {
        // if we have a model, do inference
        if (model_tester::isModelLoaded)
        {
            model_tester::update(buffer);
        }
    }

    if (currentState == IMU_DATA_PROVIDER || currentState == INFERENCE_AND_DATA_PROVIDER)
    {
        // provide data to IMU trainer
        //dataProviderTxChar.writeValue(buffer, bufferSize * FLOAT_BYTE_SIZE);
        pDataProviderTxChar->setValue((uint8_t *)buffer, bufferSize * FLOAT_BYTE_SIZE);
        pDataProviderTxChar->notify();
    }
}

unsigned long currentTime = 0;
unsigned long lastTimeDataWasUpdated = 0;
uint16_t dataDelay = 40;

void loop()
{
    currentTime = millis();

    switch (currentState)
    {
    case FILE_TRANSFER:
        updateFileTransfer();
        break;

    case CALIBRATION:
    case IMU_DATA_PROVIDER:
    case INFERENCE_AND_DATA_PROVIDER:
    case INFERENCE:
        if (!ble_file_transfer::isTransfering())
        {
            if (currentTime >= lastTimeDataWasUpdated + dataDelay)
            {
                lastTimeDataWasUpdated += dataDelay;

                if (isServerConnected && isBnoAwake)
                {
                    updateIMU();
                }
            }
        }
        break;
    default:
        break;
    }
}