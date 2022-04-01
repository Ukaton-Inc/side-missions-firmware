#include "bleWifi.h"
#include "wifi/wifi.h"

namespace bleWifi
{
    BLECharacteristic *pSSIDCharacteristic;
    class SSIDCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue();
            wifi::setSSID(data.c_str());

            auto ssid = wifi::getSSID();
            pCharacteristic->setValue(*ssid);
        }
    };

    BLECharacteristic *pPasswordCharacteristic;
    class PasswordCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue();
            wifi::setPassword(data.c_str());

            auto password = wifi::getPassword();
            pCharacteristic->setValue(*password);
        }
    };

    BLECharacteristic *pConnectCharacteristic;
    class ConnectCharacteristicCallbacks : public BLECharacteristicCallbacks
    {
        void onWrite(BLECharacteristic *pCharacteristic)
        {
            auto data = pCharacteristic->getValue();
            auto shouldConnect = (data.c_str()[0] != 0);
            if (shouldConnect)
            {
                wifi::connect();
            }
            else
            {
                wifi::disconnect();
            }

            auto autoConnect = wifi::getAutoConnect();
            pCharacteristic->setValue(autoConnect);
        }
    };

    BLECharacteristic *pIsConnectedCharacteristic;
    bool _isConnected = false;

    BLECharacteristic *pIPAddressCharacteristic;
    BLECharacteristic *pMACAddressCharacteristic;

    void setup()
    {
        pSSIDCharacteristic = ble::createCharacteristic(GENERATE_UUID("7001"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "wifi ssid");
        pSSIDCharacteristic->setValue(*wifi::getSSID());
        pSSIDCharacteristic->setCallbacks(new SSIDCharacteristicCallbacks());

        pPasswordCharacteristic = ble::createCharacteristic(GENERATE_UUID("7002"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "wifi password");
        pPasswordCharacteristic->setValue(*wifi::getPassword());
        pPasswordCharacteristic->setCallbacks(new PasswordCharacteristicCallbacks());

        auto autoConnect = wifi::getAutoConnect();
        pConnectCharacteristic = ble::createCharacteristic(GENERATE_UUID("7003"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::WRITE, "connect wifi");
        pConnectCharacteristic->setValue(autoConnect);
        pConnectCharacteristic->setCallbacks(new ConnectCharacteristicCallbacks());

        _isConnected = WiFi.isConnected();
        pIsConnectedCharacteristic = ble::createCharacteristic(GENERATE_UUID("7004"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "is wifi connected");
        pIsConnectedCharacteristic->setValue(_isConnected);

        pIPAddressCharacteristic = ble::createCharacteristic(GENERATE_UUID("7005"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "wifi IP address");
        auto ip = WiFi.localIP();
        auto ipString = ip.toString();

        pIPAddressCharacteristic->setValue(ipString);
        pMACAddressCharacteristic = ble::createCharacteristic(GENERATE_UUID("7006"), NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY, "wifi MAC address");
        auto macAddress = WiFi.macAddress();
        pMACAddressCharacteristic->setValue(macAddress);
    }

    constexpr uint16_t check_connection_delay_ms = 1000;
    unsigned long latestConnectionCheckTimestamp = 0;
    void _checkConnectionLoop()
    {
        auto isConnected = WiFi.isConnected();
        if (_isConnected != isConnected)
        {
            Serial.print("updating ble wifi connection status: ");
            Serial.println(isConnected);

            _isConnected = isConnected;

            pIsConnectedCharacteristic->setValue(_isConnected);
            pIsConnectedCharacteristic->notify();

            if (_isConnected)
            {
                auto ip = WiFi.localIP();
                auto ipString = ip.toString();
                pIPAddressCharacteristic->setValue(ipString);
                pIPAddressCharacteristic->notify();

                auto macAddress = WiFi.macAddress();
                pMACAddressCharacteristic->setValue(macAddress);
                pMACAddressCharacteristic->notify();

            }
        }
    }

    unsigned long currentTime = 0;
    void loop()
    {
        currentTime = millis();
        if (currentTime - latestConnectionCheckTimestamp > check_connection_delay_ms)
        {
            latestConnectionCheckTimestamp = currentTime - (currentTime % check_connection_delay_ms);
            _checkConnectionLoop();
        }
    }
} // namespace bleWifi
