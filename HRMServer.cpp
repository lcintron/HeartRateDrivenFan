/*
 * Heart Rate Monitor (HRM) Server
 * HRMServer.cpp
 * Author: 1cintron
 */
#include "HRMServer.h"
#include "Arduino.h"
#define DEBUG

HRMServer::HRMServer() : _serviceUUID(BLEUUID((uint16_t)0x180D)),
                         _charUUID(BLEUUID((uint16_t)0x2A37))
{
  _bpmPacket[0] = 22;
  _bpmPacket[1] = 0;
  _bpmPacketLength = 2;
  _deviceConnected = false;
  _oldDeviceConnected = false;
  _hrmServerCallback = NULL;
}

HRMServer::~HRMServer()
{
  delete _ble2902;
  delete _hrmServerCallback;
}

bool HRMServer::getDeviceConnected()
{
  return _deviceConnected;
}

bool HRMServer::getNotificationsSubscribed()
{
  return _ble2902->getNotifications();
}

void HRMServer::start()
{
  BLEDevice::init("HRM Proxy");
  _pServer = BLEDevice::createServer();
  if (_hrmServerCallback == NULL)
    _hrmServerCallback = new HRMServerCallbacks(this);

  _pServer->setCallbacks(_hrmServerCallback);
  _pService = _pServer->createService(_serviceUUID);
  _pCharacteristic = _pService->createCharacteristic(
      _charUUID,
      BLECharacteristic::PROPERTY_NOTIFY);
  _ble2902 = new BLE2902();
  _pCharacteristic->addDescriptor(_ble2902);
  _pService->start();
  _pAdvertising = BLEDevice::getAdvertising();
  _pAdvertising->addServiceUUID(_serviceUUID);
  _pAdvertising->setScanResponse(false);
  _pAdvertising->setMinPreferred(0x0);
  BLEDevice::startAdvertising();

#ifdef DEBUG
  Serial.println("Proxy adverstising...");
#endif
}

void HRMServer::disconnect()
{
  //TO BE IMPLEMENTED;
  delete _hrmServerCallback;
  _hrmServerCallback = NULL;
}

void HRMServer::setDeviceConnected(bool connected)
{
  _deviceConnected = connected;
  if (connected)
    _oldDeviceConnected = true;
}

void HRMServer::loop()
{

  //re-advertise on disconnect;
  if (!_deviceConnected && _oldDeviceConnected)
  {
#ifdef DEBUG
    Serial.println("Restarted advertising");
#endif
    _pServer->startAdvertising(); // restart advertising
    _oldDeviceConnected = false;
  }
}

void HRMServer::notify()
{
  if (_deviceConnected && _ble2902->getNotifications())
  {
#ifdef DEBUG
    Serial.print("Notified ");
    Serial.println(_bpmPacket[1]);
#endif
    _pCharacteristic->setValue(_bpmPacket, _bpmPacketLength);
    _pCharacteristic->notify();
  }
}

void HRMServer::updateBpmPacket(uint8_t *senderPacket, size_t length)
{
  _bpmPacketLength = length;
  for (size_t i = 0; i < length; i++)
  {
    _bpmPacket[i] = senderPacket[i];
  }

  this->notify();
}

//HRMServer Callbacks Class
HRMServerCallbacks::HRMServerCallbacks(HRMServer *hrmServer)
{
  _hrmServer = hrmServer;
}

void HRMServerCallbacks::onConnect(BLEServer *pServer)
{
#ifdef DEBUG
  Serial.println("************Client connected to proxy************");
#endif
  _hrmServer->setDeviceConnected(true);
}

void HRMServerCallbacks::onDisconnect(BLEServer *pServer)
{
#ifdef DEBUG
  Serial.println("************Client disconnected from proxy************");
#endif
  _hrmServer->setDeviceConnected(false);
}
