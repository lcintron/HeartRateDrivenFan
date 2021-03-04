/*
 * Heart Rate Monitor (HRM) Client
 * HRMClient.h
 * Author: 1cintron
 */
#ifndef HRMClient_h
#define HRMClient_h
#include <BLEDevice.h>

enum class HRMClientState {SCANNING, CONNECTING, CONNECTED}; 

class HRMClient: public BLEClientCallbacks, public BLEAdvertisedDeviceCallbacks {
  public:
    HRMClient(unsigned int scanTime=5);
    ~HRMClient();
    void      start();
    void      stopScan();
    void      disconnect();
    void      loop();
    bool      connectToHrmServer();
    void      updateBpmPacket(uint8_t *senderPacket, size_t length);
    bool      isConnectedToSensor();
    uint8_t   getBPM();
    uint8_t*  getBPMPacket();
    size_t    getBPMPacketLength();
    BLEUUID   getServiceUUID();
    BLEUUID   getCharacteristicUUID();
    void      onConnect(BLEClient* pclient);
    void      onDisconnect(BLEClient* pclient);
    void      onResult(BLEAdvertisedDevice advertisedDevice);

  private:

  	static void _remoteNotificationCallback(
                    BLERemoteCharacteristic * pBLERemoteCharacteristic, 
                    uint8_t *pData,	size_t packetLength,
  	                bool isNotify);
    void        requestServerNotification();
    void        doConnect();    
    static uint8_t           _bpmPacket[6];
    static size_t            _bpmPacketLength;
  	bool                     _doConnect;
  	bool                     _connected;
  	bool                     _doScan;
  	bool                     _notification;
    bool                     _lock;
    unsigned int             _scanTime;
    HRMClientState           _state;
    BLEUUID                  _serviceUUID;
    BLEUUID                  _charUUID;
    BLEScan*                 _pBLEScan;
    BLERemoteCharacteristic* _pRemoteCharacteristic;
  	BLEAdvertisedDevice*     _hrmDevice;
};
#endif
