#ifndef SMART_HOME_H
#define SMART_HOME_H

#include "Arduino.h"
#include <Bounce2.h>
#include <DHT.h>

class SmartLight {
public:
  int lightId = -1;
  int pirSwitchId = -1;

  SmartLight(int relayPin);
  SmartLight(int relayPin, int buttonPin);
  SmartLight(int relayPin, int buttonPin, int pirSensorPin);
  SmartLight(int relayPin, int buttonPin, int pirSensorPin, bool pirSwitch);

  void initialize();

  void turnLightOn(bool sendStateToController = true);
  void turnLightOff(bool sendStateToController = true);
  void turnPirSwitchOn();
  void turnPirSwitchOff();
  void turnLightOnByPir();
  void turnLightOffByPir();

  void changeLightState();

  bool isButtonActive();
  bool isPirActive();
  bool hasPirSwitch();

  bool shouldTurnLightOnByPir();
  bool shouldTurnLightOffByPir();
private:
  int _relayPin = -1;
  int _buttonPin = -1;
  int _pirSensorPin = -1;
  bool _pirSwitch = false;
  unsigned long _pirActivatedTime = 0;
  unsigned long _pirGracePeriodStart = 0;
  bool _isLightOn = false;
  bool _isLightOnByPir = false;
  bool _isPirSwitchOn = true;
  Bounce _debounce = Bounce();

  void setLightInitialState();
  void setPirSwitchInitialState();
  void sendLightStateToController();
  void sendPirSwitchStateToController();
};

class SmartDHT {
public:
  int id = -1;

  SmartDHT(int dhtPin);
  void updateTemperature();
private:
  const DHT _dht;
  unsigned long _lastUpdateTime = 0;
};

class SmartHome {
public:
  SmartHome(SmartLight* smartLights, int collectionSize);
  SmartHome(SmartLight* smartLights, int collectionSize, SmartDHT* smartDHTs, int collectionSize2);
  void beSmart();
  void doPresentation();
  void handleLightMessage(int lightId, int newState);
  void handlePirSwitchMessage(int pirSwitchId, int newState);
private:
  SmartLight* _smartLights;
  int _collectionSize = 0;
  bool _isNotInitialized = true;

  SmartDHT* _smartDHTs;
  int _collectionSize2 = 0;

  void initialize();
  void assignIds();
};

#endif
