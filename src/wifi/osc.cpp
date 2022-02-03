#include "definitions.h"
#include "wifiServer.h"
#include "ble/ble.h"
#include "osc.h"
#include "information/type.h"
#include "information/name.h"

#include "sensor/motionSensor.h"
#include "sensor/pressureSensor.h"

#include <OSCBundle.h>

namespace osc
{
    std::map<std::string, type::Type> stringToType{
        {"motionModule", type::Type::MOTION_MODULE},
        {"leftInsole", type::Type::LEFT_INSOLE},
        {"rightInsole", type::Type::RIGHT_INSOLE}};
    std::string typeStrings[(uint8_t)type::Type::COUNT]{
        "motionModule",
        "leftInsole",
        "rightInsole"};

    enum class MessageType : uint8_t
    {
        PING,
        BATTERY_LEVEL,
        TYPE,
        NAME,
        MOTION_CALIBRATION,
        SENSOR_RATE,
        SENSOR_DATA
    };

    enum class SensorType : uint8_t
    {
        MOTION,
        PRESSURE,
        COUNT
    };
    std::map<std::string, SensorType> stringToSensorType{
        {"motion", SensorType::MOTION},
        {"pressure", SensorType::PRESSURE}};
    std::string sensorTypeStrings[(uint8_t)type::Type::COUNT]{
        "motion",
        "pressure"};

    enum class MotionSensorDataType : uint8_t
    {
        ACCELERATION,
        GRAVITY,
        LINEAR_ACCELERATION,
        ROTATION_RATE,
        MAGNETOMETER,
        QUATERNION,
        COUNT
    };
    std::map<std::string, MotionSensorDataType> stringToMotionSensorDataType{
        {"acceleration", MotionSensorDataType::ACCELERATION},
        {"gravity", MotionSensorDataType::GRAVITY},
        {"linearAcceleration", MotionSensorDataType::LINEAR_ACCELERATION},
        {"rotationRate", MotionSensorDataType::ROTATION_RATE},
        {"magnetometer", MotionSensorDataType::MAGNETOMETER},
        {"quaternion", MotionSensorDataType::QUATERNION}};
    std::string motionSensorDataTypeStrings[(uint8_t)MotionSensorDataType::COUNT]{
        "acceleration",
        "gravity",
        "linearAcceleration",
        "rotationRate",
        "magnetometer",
        "quaternion"};

    enum class PressureSensorDataType : uint8_t
    {
        RAW,
        CENTER_OF_MASS,
        SUM,
        COUNT
    };
    std::map<std::string, PressureSensorDataType> stringToPressureSensorDataType{
        {"raw", PressureSensorDataType::RAW},
        {"centerOfMass", PressureSensorDataType::CENTER_OF_MASS},
        {"sum", PressureSensorDataType::SUM}};
    std::string pressureSensorDataTypeStrings[(uint8_t)PressureSensorDataType::COUNT]{
        "raw",
        "centerOfMass",
        "sum"};

    const uint16_t min_delay_ms = 20;
    uint16_t motionDataRates[(uint8_t)MotionSensorDataType::COUNT]{0};
    uint16_t pressureDataRates[(uint8_t)PressureSensorDataType::COUNT]{0};
    std::map<MotionSensorDataType, bool> _updatedMotionDataRateFlags;
    std::map<PressureSensorDataType, bool> _updatedPressureDataRateFlags;
    std::map<MotionSensorDataType, bool> _motionSensorDataFlags;
    std::map<PressureSensorDataType, bool> _pressureSensorDataFlags;

    std::map<MessageType, bool> _listenerMessageFlags;
    bool shouldSendToListener = false;

    bool hasAtLeastOneNonzeroRate = false;
    void updateHasAtLeastOneNonzeroRate()
    {
        hasAtLeastOneNonzeroRate = false;

        for (uint8_t i = 0; i < (uint8_t)MotionSensorDataType::COUNT && !hasAtLeastOneNonzeroRate; i++)
        {
            if (motionDataRates[i] != 0)
            {
                hasAtLeastOneNonzeroRate = true;
            }
        }

        for (uint8_t i = 0; i < (uint8_t)PressureSensorDataType::COUNT && !hasAtLeastOneNonzeroRate; i++)
        {
            if (pressureDataRates[i] != 0)
            {
                hasAtLeastOneNonzeroRate = true;
            }
        }
    }
    void clearRates()
    {
        memset(motionDataRates, 0, sizeof(motionDataRates));
        memset(pressureDataRates, 0, sizeof(pressureDataRates));
        updateHasAtLeastOneNonzeroRate();
    }

    unsigned long currentTime;
    unsigned long lastDataUpdateTime;
    void updateMotionSensorDataFlags()
    {
        for (uint8_t dataTypeIndex = 0; dataTypeIndex < (uint8_t)MotionSensorDataType::COUNT; dataTypeIndex++)
        {
            const uint16_t rate = motionDataRates[dataTypeIndex];
            if (rate != 0 && ((lastDataUpdateTime % rate) == 0))
            {
                auto dataType = (MotionSensorDataType)dataTypeIndex;
                _motionSensorDataFlags[dataType] = true;
            }
        }

        if (_motionSensorDataFlags.size() > 0)
        {
            _listenerMessageFlags[MessageType::SENSOR_DATA] = true;
            shouldSendToListener = true;
        }
    }
    void updatePressureSensorDataFlags()
    {
        for (uint8_t dataTypeIndex = 0; dataTypeIndex < (uint8_t)PressureSensorDataType::COUNT; dataTypeIndex++)
        {
            const uint16_t rate = pressureDataRates[dataTypeIndex];
            if (rate != 0 && ((lastDataUpdateTime % rate) == 0))
            {
                auto dataType = (PressureSensorDataType)dataTypeIndex;
                _pressureSensorDataFlags[dataType] = true;
            }
        }

        if (_pressureSensorDataFlags.size() > 0)
        {
            _listenerMessageFlags[MessageType::SENSOR_DATA] = true;
            shouldSendToListener = true;
        }
    }
    void updateSensorDataFlags()
    {
        updateMotionSensorDataFlags();
        if (type::isInsole())
        {
            updatePressureSensorDataFlags();
        }
    }

    void sensorDataLoop()
    {
        if (hasAtLeastOneNonzeroRate && currentTime >= lastDataUpdateTime + min_delay_ms)
        {
            updateSensorDataFlags();
            lastDataUpdateTime = currentTime - (currentTime % min_delay_ms);
        }
    }

    // UDP
    AsyncUDP udp;

    bool _hasListener = false;
    bool hasListener()
    {
        return _hasListener;
    }

    IPAddress remoteIP;
    uint16_t remotePort;
    unsigned long lastTimePacketWasReceivedByListener;

    void _onListenerConnection()
    {
        ble::pServer->stopAdvertising();
        Serial.println("[osc] listener connected");

        _listenerMessageFlags[MessageType::TYPE] = true;
        _listenerMessageFlags[MessageType::NAME] = true;
        shouldSendToListener = true;
    }
    void _onListenerDisconnection()
    {
        ble::pServer->startAdvertising();
        Serial.println("[osc] listener disconnected");
        clearRates();
    }

    void routeType(OSCMessage &message, int addressOffset)
    {
        if (message.isString(0))
        {
            char newTypeStringBuffer[20];
            auto length = message.getString(0, newTypeStringBuffer, sizeof(newTypeStringBuffer));
            if (length > 0)
            {
                std::string newTypeString = newTypeStringBuffer;
                if (stringToType.count(newTypeString))
                {
                    auto newType = stringToType[newTypeStringBuffer];
                    type::setType(newType);
                }
            }
        }
        _listenerMessageFlags[MessageType::TYPE] = true;
    }

    void routeName(OSCMessage &message, int addressOffset)
    {
        if (message.isString(0))
        {
            char newName[name::MAX_NAME_LENGTH];
            auto length = message.getString(0, newName, sizeof(newName));
            name::setName(newName, length);
        }
        _listenerMessageFlags[MessageType::NAME] = true;
    }

    void routeSensorRate(OSCMessage &message, int addressOffset)
    {
        if (message.isString(0) && message.isString(1) && message.isInt(2))
        {
            char sensorTypeString[10];
            auto sensorTypeStringLength = message.getString(0, sensorTypeString, sizeof(sensorTypeString));

#if DEBUG
            Serial.print("sensor type string length: ");
            Serial.println(sensorTypeStringLength);
#endif

            if (sensorTypeStringLength > 0 && stringToSensorType.count(sensorTypeString))
            {
#if DEBUG
                Serial.print("sensor type string: ");
                Serial.println(sensorTypeString);
#endif

                auto sensorType = stringToSensorType[sensorTypeString];

#if DEBUG
                Serial.print("sensor type: ");
                Serial.println((uint8_t)sensorType);
#endif

                char sensorDataTypeString[20];
                auto sensorDataTypeStringLength = message.getString(1, sensorDataTypeString, sizeof(sensorDataTypeString));

#if DEBUG
                Serial.print("sensor data type string length: ");
                Serial.println(sensorDataTypeStringLength);
#endif

                if (sensorDataTypeStringLength > 0)
                {
#if DEBUG
                    Serial.print("sensor data type string: ");
                    Serial.println(sensorDataTypeString);
#endif

                    uint16_t rate = message.getInt(2);
                    rate -= (rate % min_delay_ms);

#if DEBUG
                    Serial.print("rate: ");
                    Serial.println(rate);
#endif

                    bool updatedRate = false;

                    switch (sensorType)
                    {
                    case SensorType::MOTION:
                        if (stringToMotionSensorDataType.count(sensorDataTypeString))
                        {
                            auto motionSensorDataType = stringToMotionSensorDataType[sensorDataTypeString];
                            if (motionDataRates[(uint8_t)motionSensorDataType] != rate)
                            {
                                motionDataRates[(uint8_t)motionSensorDataType] = rate;
                                _updatedMotionDataRateFlags[motionSensorDataType] = true;
                                updatedRate = true;
                            }
                        }
                        break;
                    case SensorType::PRESSURE:
                        if (stringToPressureSensorDataType.count(sensorDataTypeString))
                        {
#if DEBUG
                            Serial.print("sensor data type: ");
                            Serial.println(sensorDataTypeString);
#endif

                            auto pressureSensorDataType = stringToPressureSensorDataType[sensorDataTypeString];
#if DEBUG
                            Serial.print("sensor data type: ");
                            Serial.println((uint8_t)pressureSensorDataType);
#endif
                            if (pressureDataRates[(uint8_t)pressureSensorDataType] != rate)
                            {
                                pressureDataRates[(uint8_t)pressureSensorDataType] = rate;
#if DEBUG
                                Serial.print("rate: ");
                                Serial.println(pressureDataRates[(uint8_t)pressureSensorDataType]);
#endif
                                _updatedPressureDataRateFlags[pressureSensorDataType] = true;
                                updatedRate = true;
                            }
                        }
                        break;
                    default:
                        break;
                    }

                    if (updatedRate)
                    {
                        _listenerMessageFlags[MessageType::SENSOR_RATE] = true;
                        shouldSendToListener = true;
                        updateHasAtLeastOneNonzeroRate();
                    }
                }
            }
        }
    }

    bool _isParsingPacket = false;
    void onUDPPacket(AsyncUDPPacket packet)
    {
        _isParsingPacket = true;
#if DEBUG
        Serial.print("UDP Packet Type: ");
        Serial.print(packet.isBroadcast() ? "Broadcast" : (packet.isMulticast() ? "Multicast" : "Unicast"));
        Serial.print(", From: ");
        Serial.print(packet.remoteIP());
        Serial.print(":");
        Serial.print(packet.remotePort());
        Serial.print(", To: ");
        Serial.print(packet.localIP());
        Serial.print(":");
        Serial.print(packet.localPort());
        Serial.print(", Length: ");
        Serial.print(packet.length());
        Serial.print(", Data: ");
        auto packetData = packet.data();
        for (uint8_t index = 0; index < packet.length(); index++)
        {
            Serial.print(packetData[index]);
            Serial.print(", ");
        }
        Serial.println();
#endif

        lastTimePacketWasReceivedByListener = currentTime;
        if (!_hasListener)
        {
            _hasListener = true;
            remoteIP = packet.remoteIP();
            remotePort = packet.remotePort();
            _onListenerConnection();
        }
        else
        {
            if (packet.remoteIP() != remoteIP || packet.remotePort() != remotePort)
            {
                Serial.println("not the same IP!");
                return;
            }
        }

        OSCBundle bundle;
        bundle.fill(packet.data(), packet.length());
        if (!bundle.hasError())
        {
            bundle.route("/type", routeType);
            bundle.route("/name", routeName);
            bundle.route("/rate", routeSensorRate);
            shouldSendToListener = shouldSendToListener || (_listenerMessageFlags.size() > 0);
        }

        _isParsingPacket = false;
    }

    const uint16_t listenerTimeout = 4000; // ping every <4 seconds
    void checkListenerConnection()
    {
        if (currentTime - lastTimePacketWasReceivedByListener > listenerTimeout)
        {
            _hasListener = false;
            _onListenerDisconnection();
        }
    }

    void listen(uint16_t port)
    {
        if (udp.listen(port))
        {
            Serial.print("OSC Listening on IP: ");
            Serial.print(WiFi.localIP());
            Serial.print(": ");
            Serial.println(port);
            udp.onPacket([](AsyncUDPPacket packet)
                         { onUDPPacket(packet); });
        }
    }

    void batteryLevelLoop()
    {
        // FIX LATER
    }
    unsigned long lastCalibrationUpdateTime;
    void motionCalibrationLoop()
    {
        if (lastCalibrationUpdateTime != motionSensor::lastCalibrationUpdateTime)
        {
            lastCalibrationUpdateTime = motionSensor::lastCalibrationUpdateTime;
            _listenerMessageFlags[MessageType::MOTION_CALIBRATION] = true;
            shouldSendToListener = true;
        }
    }

    // SENDING DATA
    uint16_t remotePortSend = 9998;
    OSCBundle bundleOUT;
    AsyncUDPMessage message;
    void messageLoop()
    {
        if (shouldSendToListener)
        {
            for (auto listenerMessageFlagIterator = _listenerMessageFlags.begin(); listenerMessageFlagIterator != _listenerMessageFlags.end(); listenerMessageFlagIterator++)
            {
                auto messageType = listenerMessageFlagIterator->first;

                switch (messageType)
                {
                case MessageType::TYPE:
                {
                    auto type = type::getType();
                    auto typeString = typeStrings[(uint8_t)type];
                    bundleOUT.add("/type").add(typeString.c_str());
                }
                break;
                case MessageType::NAME:
                    bundleOUT.add("/name").add(name::getName()->c_str());
                    break;
                case MessageType::MOTION_CALIBRATION:
                    bundleOUT.add("/calibration").add("motion").add("system").add(motionSensor::calibration[0]);
                    bundleOUT.add("/calibration").add("motion").add("gyroscope").add(motionSensor::calibration[1]);
                    bundleOUT.add("/calibration").add("motion").add("accelerometer").add(motionSensor::calibration[2]);
                    bundleOUT.add("/calibration").add("motion").add("magnetometer").add(motionSensor::calibration[3]);
                    break;
                case MessageType::SENSOR_RATE:
                    for (auto updatedMotionDataRateFlagIterator = _updatedMotionDataRateFlags.begin(); updatedMotionDataRateFlagIterator != _updatedMotionDataRateFlags.end(); updatedMotionDataRateFlagIterator++)
                    {
                        auto motionDataType = updatedMotionDataRateFlagIterator->first;
                        auto rate = motionDataRates[(uint8_t)motionDataType];
                        auto motionDataTypeString = motionSensorDataTypeStrings[(uint8_t)motionDataType];
                        bundleOUT.add("/rate").add("motion").add(motionDataTypeString.c_str()).add(rate);
                    }
                    _updatedMotionDataRateFlags.clear();

                    for (auto updatedPressureDataRateFlagIterator = _updatedPressureDataRateFlags.begin(); updatedPressureDataRateFlagIterator != _updatedPressureDataRateFlags.end(); updatedPressureDataRateFlagIterator++)
                    {
                        auto pressureDataType = updatedPressureDataRateFlagIterator->first;
                        auto rate = pressureDataRates[(uint8_t)pressureDataType];
                        auto pressureDataTypeString = pressureSensorDataTypeStrings[(uint8_t)pressureDataType];
                        bundleOUT.add("/rate").add("pressure").add(pressureDataTypeString.c_str()).add(rate);
                    }
                    _updatedPressureDataRateFlags.clear();
                    break;
                case MessageType::SENSOR_DATA:
                {
                    for (auto motionSensorDataFlagIterator = _motionSensorDataFlags.begin(); motionSensorDataFlagIterator != _motionSensorDataFlags.end(); motionSensorDataFlagIterator++)
                    {
                        auto motionDataType = motionSensorDataFlagIterator->first;

                        switch (motionDataType)
                        {
                        case MotionSensorDataType::ACCELERATION:
                        {
                            auto vector = motionSensor::bno.getVector(Adafruit_BNO055::VECTOR_ACCELEROMETER);
                            bundleOUT.add("/data").add("motion").add("acceleration").add((float)vector.x()).add((float)vector.y()).add((float)vector.z());
                        }
                        break;
                        case MotionSensorDataType::GRAVITY:
                        {
                            auto vector = motionSensor::bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
                            bundleOUT.add("/data").add("motion").add("gravity").add((float)vector.x()).add((float)vector.y()).add((float)vector.z());
                        }
                        break;
                        case MotionSensorDataType::LINEAR_ACCELERATION:
                        {
                            auto vector = motionSensor::bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
                            bundleOUT.add("/data").add("motion").add("linearAcceleration").add((float)vector.x()).add((float)vector.y()).add((float)vector.z());
                        }
                        break;
                        case MotionSensorDataType::ROTATION_RATE:
                        {
                            auto vector = motionSensor::bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
                            bundleOUT.add("/data").add("motion").add("gravity").add((float)vector.x()).add((float)vector.y()).add((float)vector.z());
                        }
                        break;
                        case MotionSensorDataType::MAGNETOMETER:
                        {
                            auto vector = motionSensor::bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
                            bundleOUT.add("/data").add("motion").add("magnetometer").add((float)vector.x()).add((float)vector.y()).add((float)vector.z());
                        }
                        break;
                        case MotionSensorDataType::QUATERNION:
                        {
                            auto quaternion = motionSensor::bno.getQuat();
                            bundleOUT.add("/data").add("motion").add("quaternion").add((float)quaternion.w()).add((float)quaternion.x()).add((float)quaternion.y()).add((float)quaternion.z());
                        }
                        break;
                        default:
                            break;
                        }
                    }
                    _motionSensorDataFlags.clear();

                    if (_pressureSensorDataFlags.size() > 0)
                    {
                        pressureSensor::update();
                    }
                    for (auto pressureSensorDataFlagIterator = _pressureSensorDataFlags.begin(); pressureSensorDataFlagIterator != _pressureSensorDataFlags.end(); pressureSensorDataFlagIterator++)
                    {
                        auto pressureDataType = pressureSensorDataFlagIterator->first;
#if DEBUG
                        Serial.print("packing pressure data type: ");
                        Serial.println((uint8_t)pressureDataType);
#endif
                        switch (pressureDataType)
                        {
                        case PressureSensorDataType::RAW:
                        {
                            auto pressureData = pressureSensor::getPressureDataDoubleByte();
                            auto message = *(new OSCMessage("/data"));
                            message.add("pressure").add("raw");
                            auto scalar = (float) pow(2, 12);
                            for (uint8_t index = 0; index < pressureSensor::number_of_pressure_sensors; index++)
                            {
                                float value = (float)(pressureData[index]);
                                value /= scalar;
#if DEBUG
                                Serial.print(index);
                                Serial.print(": ");
                                Serial.println(value);
#endif
                                message.add(value);
                            }
                            bundleOUT.add(message);
                        }
                        break;
                        case PressureSensorDataType::CENTER_OF_MASS:
                        {
                            auto centerOfMass = pressureSensor::getCenterOfMass();
                            bundleOUT.add("/data").add("pressure").add("centerOfMass").add(centerOfMass[0]).add(1 - centerOfMass[1]);
                        }
                        break;
                        case PressureSensorDataType::SUM:
                        {
                            auto sum = *(pressureSensor::getMass());
                            float scalar = (float)pow(2, 12) * (float)pressureSensor::number_of_pressure_sensors;
                            float normalizedSum = (float)sum / scalar;
                            bundleOUT.add("/data").add("pressure").add("sum").add(normalizedSum);
                        }
                        break;
                        default:
                            break;
                        }
                    }
                    _pressureSensorDataFlags.clear();
                }
                break;
                default:
                    Serial.print("uncaught listener message type: ");
                    Serial.println((uint8_t)messageType);
                    break;
                }
            }

#if DEBUG
            Serial.println("about to send...");
#endif
            bundleOUT.send(message);
            udp.sendTo(message, remoteIP, remotePortSend);
            message.flush();
            bundleOUT.empty();
#if DEBUG
            Serial.println("sent!");
#endif

            shouldSendToListener = false;
            _listenerMessageFlags.clear();
        }
    }

    void loop()
    {
        currentTime = millis();
        if (_hasListener && !_isParsingPacket)
        {
            checkListenerConnection();
            batteryLevelLoop();
            motionCalibrationLoop();
            sensorDataLoop();
            messageLoop();
        }
    }
} // namespace osc
