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
        EULER_ANGLES,
        COUNT
    };
    std::map<std::string, MotionSensorDataType> stringToMotionSensorDataType{
        {"acceleration", MotionSensorDataType::ACCELERATION},
        {"gravity", MotionSensorDataType::GRAVITY},
        {"linearAcceleration", MotionSensorDataType::LINEAR_ACCELERATION},
        {"rotationRate", MotionSensorDataType::ROTATION_RATE},
        {"magnetometer", MotionSensorDataType::MAGNETOMETER},
        {"quaternion", MotionSensorDataType::QUATERNION},
        {"euler", MotionSensorDataType::EULER_ANGLES}};
    std::string motionSensorDataTypeStrings[(uint8_t)MotionSensorDataType::COUNT]{
        "acceleration",
        "gravity",
        "linearAcceleration",
        "rotationRate",
        "magnetometer",
        "quaternion",
        "euler"};

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

            if (sensorTypeStringLength > 0 && stringToSensorType.count(sensorTypeString))
            {
                auto sensorType = stringToSensorType[sensorTypeString];

                char sensorDataTypeString[20];
                auto sensorDataTypeStringLength = message.getString(1, sensorDataTypeString, sizeof(sensorDataTypeString));

                if (sensorDataTypeStringLength > 0)
                {
                    uint16_t rate = message.getInt(2);
                    rate -= (rate % min_delay_ms);

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
    void rearrangeVector(float *vector)
    {
        auto x = vector[0];
        auto y = vector[1];
        auto z = vector[2];

        auto _type = type::getType();
        switch (_type)
        {
        case type::Type::MOTION_MODULE:
            vector[0] = x;
            vector[1] = -z;
            vector[2] = -y;
            break;
        case type::Type::LEFT_INSOLE:
            vector[0] = -z;
            vector[1] = y;
            vector[2] = -x;
            break;
        case type::Type::RIGHT_INSOLE:
            vector[0] = z;
            vector[1] = y;
            vector[2] = x;
            break;
        default:
            break;
        }
    }
    void rearrangeEuler(float *euler)
    {
        auto x = euler[0];
        auto y = euler[1];
        auto z = euler[2];

        auto _type = type::getType();
        switch (_type)
        {
        case type::Type::MOTION_MODULE:
            euler[0] = -x;
            euler[1] = z;
            euler[2] = y;
            break;
        case type::Type::LEFT_INSOLE:
            euler[0] = z;
            euler[1] = -y;
            euler[2] = x;
            break;
        case type::Type::RIGHT_INSOLE:
            euler[0] = -z;
            euler[1] = -y;
            euler[2] = -x;
            break;
        default:
            break;
        }
    }
    imu::Quaternion leftInsoleCorrectionQuaternion(0.5, -0.5, -0.5, 0.5);
    imu::Quaternion rightInsoleCorrectionQuaternion(0.5, -0.5, 0.5, -0.5);
    imu::Quaternion rearrangeQuaternion(imu::Quaternion quaternion)
    {
        auto x = quaternion.x();
        auto y = quaternion.y();
        auto z = quaternion.z();
        auto w = quaternion.w();

        auto rearrangedQuaternion = imu::Quaternion(z, -y, -w, -x);

        auto _type = type::getType();
        switch (_type)
        {
        case type::Type::LEFT_INSOLE:
            rearrangedQuaternion = rearrangedQuaternion * leftInsoleCorrectionQuaternion;
            break;
        case type::Type::RIGHT_INSOLE:
            rearrangedQuaternion = rearrangedQuaternion * rightInsoleCorrectionQuaternion;
            break;
        default:
            break;
        }

        return rearrangedQuaternion;
    }

    enum class EULER_ORDER : uint8_t
    {
        XYZ,
        YXZ,
        ZXY,
        ZYX,
        YZX,
        XZY
    };
    double clamp(double value, double _min, double _max)
    {
        return max(_min, min(_max, value));
    }

    // https://github.com/mrdoob/three.js/blob/a3ce4c9571b83eb418980a3433081bcae81350e2/src/math/Euler.js#L104
    imu::Vector<3> quaternionToEuler(imu::Quaternion quaternion, EULER_ORDER order = EULER_ORDER::XYZ)
    {
        auto matrix = quaternion.toMatrix();

        auto m11 = matrix.cell(0, 0);
        auto m21 = matrix.cell(1, 0);
        auto m31 = matrix.cell(2, 0);
        auto m12 = matrix.cell(0, 1);
        auto m22 = matrix.cell(1, 1);
        auto m32 = matrix.cell(2, 1);
        auto m13 = matrix.cell(0, 2);
        auto m23 = matrix.cell(1, 2);
        auto m33 = matrix.cell(2, 2);

        double yaw, pitch, roll;

        switch (order)
        {
        case EULER_ORDER::XYZ:
            yaw = asin(clamp(m13, -1, 1));
            if (abs(m13) < 0.9999999)
            {
                pitch = atan2(-m23, m33);
                roll = atan2(-m12, m11);
            }
            else
            {
                pitch = atan2(m32, m22);
                roll = 0;
            }
            break;

        case EULER_ORDER::YXZ:
            pitch = asin(-clamp(m23, -1, 1));
            if (abs(m23) < 0.9999999)
            {
                yaw = atan2(m13, m33);
                roll = atan2(m21, m22);
            }
            else
            {
                yaw = atan2(-m31, m11);
                roll = 0;
            }
            break;

        case EULER_ORDER::ZXY:
            pitch = asin(clamp(m32, -1, 1));
            if (abs(m32) < 0.9999999)
            {
                yaw = atan2(-m31, m33);
                roll = atan2(-m12, m22);
            }
            else
            {
                yaw = 0;
                roll = atan2(m21, m11);
            }
            break;

        case EULER_ORDER::ZYX:
            yaw = asin(-clamp(m31, -1, 1));
            if (abs(m31) < 0.9999999)
            {
                pitch = atan2(m32, m33);
                roll = atan2(m21, m11);
            }
            else
            {
                pitch = 0;
                roll = atan2(-m12, m22);
            }
            break;

        case EULER_ORDER::YZX:
            roll = asin(clamp(m21, -1, 1));
            if (abs(m21) < 0.9999999)
            {
                pitch = atan2(-m23, m33);
                yaw = atan2(-m31, m11);
            }
            else
            {
                pitch = 0;
                yaw = atan2(m13, m33);
            }
            break;

        case EULER_ORDER::XZY:
            roll = asin(-clamp(m12, -1, 1));
            if (abs(m12) < 0.9999999)
            {
                pitch = atan2(m32, m22);
                yaw = atan2(m13, m11);
            }
            else
            {
                pitch = atan2(-m23, m33);
                yaw = 0;
            }
            break;
        }

        return imu::Vector<3>(pitch, yaw, roll);
    }

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
                    auto _type = type::getType();
                    auto typeString = typeStrings[(uint8_t)_type];
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
                            float v[3]{vector.x(), vector.y(), vector.z()};
                            rearrangeVector(v);
                            bundleOUT.add("/data").add("motion").add("acceleration").add(v[0]).add(v[1]).add(v[2]);
                        }
                        break;
                        case MotionSensorDataType::GRAVITY:
                        {
                            auto vector = motionSensor::bno.getVector(Adafruit_BNO055::VECTOR_GRAVITY);
                            float v[3]{vector.x(), vector.y(), vector.z()};
                            rearrangeVector(v);
                            bundleOUT.add("/data").add("motion").add("gravity").add(v[0]).add(v[1]).add(v[2]);
                        }
                        break;
                        case MotionSensorDataType::LINEAR_ACCELERATION:
                        {
                            auto vector = motionSensor::bno.getVector(Adafruit_BNO055::VECTOR_LINEARACCEL);
                            float v[3]{vector.x(), vector.y(), vector.z()};
                            rearrangeVector(v);
                            bundleOUT.add("/data").add("motion").add("linearAcceleration").add(v[0]).add(v[1]).add(v[2]);
                        }
                        break;
                        case MotionSensorDataType::ROTATION_RATE:
                        {
                            auto vector = motionSensor::bno.getVector(Adafruit_BNO055::VECTOR_GYROSCOPE);
                            float e[3]{vector.x(), vector.y(), vector.z()};
                            rearrangeEuler(e);
                            bundleOUT.add("/data").add("motion").add("rotationRate").add(e[0]).add(e[1]).add(e[2]);
                        }
                        break;
                        case MotionSensorDataType::MAGNETOMETER:
                        {
                            auto vector = motionSensor::bno.getVector(Adafruit_BNO055::VECTOR_MAGNETOMETER);
                            float v[3]{vector.x(), vector.y(), vector.z()};
                            rearrangeVector(v);
                            bundleOUT.add("/data").add("motion").add("magnetometer").add(v[0]).add(v[1]).add(v[2]);
                        }
                        break;
                        case MotionSensorDataType::QUATERNION:
                        case MotionSensorDataType::EULER_ANGLES:
                        {
                            if (motionDataType == MotionSensorDataType::QUATERNION || _motionSensorDataFlags.count(MotionSensorDataType::QUATERNION) == 0)
                            {
                                auto quaternion = motionSensor::bno.getQuat();
                                auto rearrangedQuaternion = rearrangeQuaternion(quaternion);
                                float q[4]{rearrangedQuaternion.x(), rearrangedQuaternion.y(), rearrangedQuaternion.z(), rearrangedQuaternion.w()};

                                if (motionDataType == MotionSensorDataType::QUATERNION)
                                {
                                    bundleOUT.add("/data").add("motion").add("quaternion").add(q[0]).add(q[1]).add(q[2]).add(q[3]);
                                }
                                else
                                {
                                    auto euler = quaternionToEuler(rearrangedQuaternion, EULER_ORDER::YXZ);
                                    euler.toDegrees();
                                    float e[3]{euler.x(), euler.y(), euler.z()};
                                    bundleOUT.add("/data").add("motion").add("euler").add(e[0]).add(e[1]).add(e[2]);
                                }
                            }
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
                            auto scalar = (float)pow(2, 12);
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

            bundleOUT.send(message);
            udp.sendTo(message, remoteIP, remotePortSend);
            message.flush();
            bundleOUT.empty();

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
