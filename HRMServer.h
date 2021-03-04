/*
 * Heart Rate Monitor (HRM) Server
 * HRMServer.h
 * Author: 1cintron
 */
#ifndef HRMServer_h
#define HRMServer_h
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h>

class HRMServerCallbacks;

class HRMServer{
  public:
    HRMServer();
    ~HRMServer();
    void start();
    void disconnect();
    void notify();
    void loop();
    void updateBpmPacket(uint8_t *senderPacket, size_t length);
    void setDeviceConnected(bool connected);
    bool getDeviceConnected();
    bool getNotificationsSubscribed();
    
  private:
    BLEUUID            _serviceUUID;
    BLEUUID            _charUUID;
    BLECharacteristic* _pCharacteristic;
    BLEServer*         _pServer;
    BLEService*        _pService;
    BLEAdvertising*    _pAdvertising;
    BLE2902*           _ble2902; 
    uint8_t            _bpmPacket[6];
    size_t             _bpmPacketLength;
    bool               _deviceConnected;
    bool               _oldDeviceConnected;
    HRMServerCallbacks* _hrmServerCallback;

};

class HRMServerCallbacks: public BLEServerCallbacks {
  public:
      HRMServerCallbacks(HRMServer *hrmServer);
      void onConnect(BLEServer* pServer);
      void onDisconnect(BLEServer* pServer);
      
  private:
    HRMServer *_hrmServer;
};
#endif
