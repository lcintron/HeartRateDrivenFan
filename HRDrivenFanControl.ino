/*
 * A BLE client example that is rich in capabilities.
 * There is a lot new capabilities implemented.
 * author unknown
 * Author: 1cintron
 * Baseline example by chegewara
*/

#include "HRMClient.h"
#include "HRMServer.h"
#include <ESP32Servo.h>

Servo myservo;

int pos = 0;
int servoPin = 25;
int fanLevels[] = {140, 0, 50, 90}; //fan position angles
int hrFanZones[] = {145, 165, 180};
unsigned long servoLastMovedMillis = 0;
HRMClient *hrmClient = NULL;
HRMServer *hrmServer = NULL;

unsigned int state = 0;

unsigned long stateScanMillis = 0;
bool stateScanLED = false;
const int scanLEDPin = 32;

unsigned long stateServingMillis = 0;
bool stateServingLED = false;
const int servingLEDPin = 33;

unsigned long pushButtonMillis = 0;
const int pushButtonPin = 12;
bool pushButtonInit = false;

unsigned long hrmServerUpdateMillis = 0;

void showState()
{
  //SCANNING FOR HR DEVICE
  if (state == 0)
  {
    unsigned millisDiff = millis() - stateScanMillis;
    if (millisDiff > 500)
    {
      digitalWrite(scanLEDPin, stateScanLED ? HIGH : LOW);
      stateScanLED = !stateScanLED;
      stateScanMillis = millis();
    }
    digitalWrite(servingLEDPin, LOW);
  }
  //HR DEVICE FOUND, SERVING HR SERVICE BUT NO CLIENT
  else if (state == 1)
  {
    unsigned millisDiff = millis() - stateServingMillis;
    if (millisDiff > 500)
    {
      digitalWrite(servingLEDPin, stateServingLED ? HIGH : LOW);
      stateServingLED = !stateServingLED;
      stateServingMillis = millis();
    }
    digitalWrite(scanLEDPin, HIGH);
  }
  //HR PROXY SERVICE HAS CLIENT CONNECTED
  else if (state == 2)
  {
    digitalWrite(servingLEDPin, HIGH);
    digitalWrite(scanLEDPin, HIGH);
  }
}

void testLEDs(){
    digitalWrite(servingLEDPin, HIGH);
    digitalWrite(scanLEDPin, HIGH);
    delay(300);
    digitalWrite(servingLEDPin, LOW);
    digitalWrite(scanLEDPin, LOW);
    delay(300);
    digitalWrite(servingLEDPin, HIGH);
    digitalWrite(scanLEDPin, HIGH);
    delay(300);
    digitalWrite(servingLEDPin, LOW);
    digitalWrite(scanLEDPin, LOW);
    delay(300);
}

void setFanLevel(unsigned int level)
{
  if (level > 3)
    return;

  unsigned long millisDiff = millis() - servoLastMovedMillis;
  
  //at least 10secs since last change before changing again
  if (millisDiff > 10000) 
  {
    myservo.write(fanLevels[level]);
  }

  return;
}

int hrToFanZone(uint8_t bpm)
{
  if (bpm > hrFanZones[2])
  {
    return 3;
  }
  else if (bpm > hrFanZones[1])
  {
    return 2;
  }
  else if (bpm > hrFanZones[0])
  {
    return 1;
  }
  return 0;
}

void IRAM_ATTR PushButtonTriggerRaised() {
  if(!pushButtonInit){
    Serial.println("Pushed");
    pushButtonMillis = millis();
    pushButtonInit = true;
  }
  else{
   Serial.println("Released");
    unsigned long millisDiff = millis() - pushButtonMillis;
    if(millisDiff>2000){
      Serial.println("Long Pressed");
    }
    else if(millisDiff>500){
      Serial.println("Short Pressed");
    }
    pushButtonInit = false;
  }
}

void setupServo()
{
  ESP32PWM::allocateTimer(0);
  ESP32PWM::allocateTimer(1);
  ESP32PWM::allocateTimer(2);
  ESP32PWM::allocateTimer(3);
  myservo.setPeriodHertz(50);
  myservo.attach(servoPin, 500, 2400);
  myservo.write(fanLevels[0]);
}

void setup()
{
  //pinMode(pushButtonPin, INPUT);
  //attachInterrupt(pushButtonPin, PushButtonTriggerPressed, RISING);
  //attachInterrupt(pushButtonPin, PushButtonTriggerRaised, CHANGE);
  pinMode(servingLEDPin, OUTPUT);
  pinMode(scanLEDPin, OUTPUT);
  testLEDs();
  Serial.begin(115200);
  setupServo();
  hrmClient = new HRMClient(1);
  hrmClient->start();
}



void loop()
{
  //Serial.println("looping...");
  hrmClient->loop();
  if (hrmClient->isConnectedToSensor())
  {
    //Serial.println("client connected");
    if (hrmServer == NULL)
    {
      //Serial.println("Creating proxy service");
      hrmServer = new HRMServer();
      hrmServer->start();
    }
    uint8_t bpm = hrmClient->getBPM();
    //Serial.print("Received bpm. Setting servo to ");
    Serial.print(bpm);
    //Serial.print(" bpm, level ");
    int fanLevel = hrToFanZone(bpm);
    Serial.println(fanLevel);
    setFanLevel(fanLevel);

    unsigned long srvrMillisDiff = millis() - hrmServerUpdateMillis;
    if (srvrMillisDiff > 100)
    {
      hrmServer->updateBpmPacket(hrmClient->getBPMPacket(), hrmClient->getBPMPacketLength());
      hrmServer->notify();
      hrmServerUpdateMillis = millis();
    }

    state = !hrmServer->getNotificationsSubscribed()?1:2;
  }
  else
  {
    setFanLevel(0);
    state = 0;
    delete hrmServer;
    hrmServer = NULL;
  }

  if (hrmServer != NULL)
    hrmServer->loop();

  showState();
  delay(100);
}
