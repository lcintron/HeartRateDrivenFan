/*
 * Heart Rate Monitor (HRM) Client
 * HRMClient.cpp
 * Author: 1cintron
 */
#include "HRMClient.h"
#include "Arduino.h"
#if defined(ARDUINO_ARCH_ESP32) && defined(CONFIG_ARDUHAL_ESP_LOG)
#include "esp32-hal-log.h"
#define LOG_TAG ""
#else
#include "esp_log.h"
static const char* LOG_TAG = "HRMClient";
#endif

//#define DEBUG


size_t HRMClient::_bpmPacketLength = 0;
uint8_t HRMClient::_bpmPacket[] = {22, 0, 0, 0, 0, 0};

HRMClient::HRMClient(unsigned int scanTime) : _serviceUUID(BLEUUID((uint16_t)0x180D)),
                                              _charUUID(BLEUUID((uint16_t)0x2A37))
{
    _scanTime = scanTime;
    _connected = false;
    _doScan = true;
    _doConnect = false;
    _notification = false;
    _hrmDevice = NULL;
    _lock = false;
    _state = HRMClientState::SCANNING;
    _pBLEScan = NULL;
}

HRMClient::~HRMClient()
{
    delete _hrmDevice;
    _hrmDevice = NULL;
}

void HRMClient::start()
{
    if (_pBLEScan == NULL)
    {
        BLEDevice::init("HRM Proxy");
        _pBLEScan = BLEDevice::getScan();
        _pBLEScan->setAdvertisedDeviceCallbacks(this);
        _pBLEScan->setInterval(1349);
        _pBLEScan->setWindow(449);
        _pBLEScan->setActiveScan(true);
    }
    
    _pBLEScan->start(_scanTime);
}

void HRMClient::disconnect()
{
    _connected = false;
    _doScan = true;
    _doConnect = false;
    _notification = false;
    delete _hrmDevice;
    _hrmDevice = NULL;
    _state = HRMClientState::SCANNING;
    _lock = false;
    HRMClient::_bpmPacket[1] = 0;
    HRMClient::_bpmPacketLength = 2;
#ifdef DEBUG
    Serial.println("Disconnected funct called");
#endif
}

void HRMClient::requestServerNotification()
{
    if (_connected && !_notification)
    {
#ifdef DEBUG
        Serial.println("Turning HRM Notification On");
#endif
        const uint8_t onPacket[] = {0x01, 0x0};
        _pRemoteCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t *)onPacket, 2, true);
        _notification = true;
    }
}

void HRMClient::loop()
{
    // If we are connected to a peer BLE Server, update the characteristic each time we are reached
    // with the current time since boot.
    if (!_lock)
    {
#ifdef DEBUG
        Serial.println("critical section");
#endif
        if (_connected)  //CONNECTED
        {
#ifdef DEBUG
            Serial.println("at connected");
#endif
            requestServerNotification();
        }
        else if (_doConnect) //CONNECTING
        {
#ifdef DEBUG
            Serial.println("at doConnect");
#endif
            doConnect();
        }
        else if (_doScan) //SCANNING
        {
#ifdef DEBUG
            Serial.println("at doScan");
#endif
            BLEDevice::getScan()->start(_scanTime);
        }
    }
}

void HRMClient::updateBpmPacket(uint8_t *senderPacket, size_t length)
{
    HRMClient::_bpmPacketLength = length > 6 ? 6 : length;

    for (int i = 0; i < HRMClient::_bpmPacketLength; i++)
    {
        HRMClient::_bpmPacket[i] = senderPacket[i];
    }
}

BLEUUID HRMClient::getServiceUUID()
{
    return _serviceUUID;
}

BLEUUID HRMClient::getCharacteristicUUID()
{
    return _charUUID;
}

bool HRMClient::isConnectedToSensor()
{
    return _connected;
}

uint8_t HRMClient::getBPM()
{
    return _bpmPacket[1];
}

uint8_t *HRMClient::getBPMPacket()
{
    return _bpmPacket;
}

size_t HRMClient::getBPMPacketLength()
{
    return HRMClient::_bpmPacketLength;
}

bool HRMClient::connectToHrmServer()
{
    Serial.print("Forming a connection to ");
    Serial.println(_hrmDevice->getAddress().toString().c_str());

    BLEClient *pClient = BLEDevice::createClient();
    Serial.println("Created HR client");

    pClient->setClientCallbacks(this);
    Serial.println("SetClientCallbacks");
    bool connected = pClient->connect(_hrmDevice);
    delay(100);
    Serial.println("Connect client called");
    if (!connected)
    {
        Serial.println("Unable to connect, trying again...");
        this->disconnect();
        return connected;
    }
    Serial.println("***Connected to HRM sensor***");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService *pRemoteService = pClient->getService(_serviceUUID);
    if (pRemoteService == nullptr)
    {
        Serial.print("Failed to find our service UUID: ");
        Serial.println(_serviceUUID.toString().c_str());
        pClient->disconnect();
        return false;
    }
    Serial.println(" Found HR service");

    // Obtain a reference to the characteristic in the service of the remote BLE server.
    _pRemoteCharacteristic = pRemoteService->getCharacteristic(_charUUID);
    if (_pRemoteCharacteristic == nullptr)
    {
        Serial.print("Failed to find HR characteristic UUID: ");
        Serial.println(_charUUID.toString().c_str());
        pClient->disconnect();
        this->disconnect();
        return _connected;
    }
    Serial.println(" Found HR characteristic");

    // Read the value of the characteristic.
    if (_pRemoteCharacteristic->canRead())
    {
        std::string value = _pRemoteCharacteristic->readValue();
        Serial.print("The characteristic value was: ");
        Serial.println(value.c_str());
    }

    if (_pRemoteCharacteristic->canNotify())
        _pRemoteCharacteristic->registerForNotify(_remoteNotificationCallback);

    _connected = connected;
    this->requestServerNotification();
    return _connected;
}
void HRMClient::_remoteNotificationCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t packetLength, bool isNotify)
{
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(packetLength);
    Serial.print("pData is ");
    HRMClient::_bpmPacketLength = packetLength;
    for (int i = 0; i < packetLength; i++)
    {
        Serial.print(pData[i]);
        Serial.print("\t");
        HRMClient::_bpmPacket[i] = pData[i];
    }

    if (packetLength >= 2)
    {
        uint8_t bpm = pData[1];
        Serial.print("\nBPM: ");
        Serial.println(bpm);
    }
}

void HRMClient::onConnect(BLEClient *pclient)
{
    Serial.println("***onConnect Event***");
}

void HRMClient::onDisconnect(BLEClient *pclient)
{
    Serial.println("***onDisconnect Event***");
    this->disconnect();
    Serial.println("Client disconnected!");
}

void HRMClient::onResult(BLEAdvertisedDevice advertisedDevice)
{
   // Serial.print("BLE Advertised Device found: ");
    //Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(_serviceUUID))
    {
        Serial.println("Found HR device...");
        _hrmDevice = new BLEAdvertisedDevice(advertisedDevice);
        _state = HRMClientState::CONNECTING;
        _doConnect = true;
    }
}

void HRMClient::doConnect()
{
    if (_doConnect)
    {
        _lock = true;
        delay(101);
        _doScan = false;
        BLEDevice::getScan()->stop();
        bool connected = this->connectToHrmServer();
        _state = connected ? HRMClientState::CONNECTED : HRMClientState::SCANNING;
        _doScan = !connected;
        _doConnect = false;
        if (!connected)
        {
            this->disconnect();
        }
        _lock = false;
    }
}
