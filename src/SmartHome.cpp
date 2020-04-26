#include "Arduino.h"
#include "SmartHome.h"

//=================
//MY SENSORS CONFIG
#define MY_GATEWAY_SERIAL
#define MY_REPEATER_FEATURE
#define MY_NODE_ID 1
#if F_CPU == 8000000L
#define MY_BAUD_RATE 38400
#endif
//=================

#include <SPI.h>
#include <MySensors.h>

#define BUTTON_ACTIVE LOW
#define PIR_DETECTOR_ACTIVE_TIME 10000
#define PIR_DETECTOR_GRACE_PERIOD 10000
#define RELAY_ON LOW
#define RELAY_OFF HIGH
#define STATE_ON 1
#define STATE_OFF 0

extern SmartHome smartHome;

//=========SMART_LIGHT==========

SmartLight::SmartLight(int relayPin) {
	_relayPin = relayPin;
	pinMode(_relayPin, OUTPUT);
	digitalWrite(_relayPin, RELAY_OFF);
}

SmartLight::SmartLight(int relayPin, int buttonPin) : SmartLight(relayPin) {
	_buttonPin = buttonPin;
	_debounce.attach(_buttonPin);
	_debounce.interval(5);

	pinMode(_buttonPin, INPUT_PULLUP);
}

SmartLight::SmartLight(int relayPin, int buttonPin, int pirSensorPin) : SmartLight(relayPin, buttonPin) {
	_pirSensorPin = pirSensorPin;

	pinMode(_pirSensorPin, INPUT);
	digitalWrite(_pirSensorPin, LOW);
}

SmartLight::SmartLight(int relayPin, int buttonPin, int pirSensorPin, bool pirSwitch) : SmartLight(relayPin, buttonPin, pirSensorPin) {
	_pirSwitch = pirSwitch;
}

void SmartLight::turnLightOn(bool sendStateToController) {
	digitalWrite(_relayPin, RELAY_ON);
	saveState(lightId, STATE_ON);
	_isLightOn = true;
	_isLightOnByPir = false;
	if (sendStateToController) {
		sendLightStateToController();
	}
}

void SmartLight::turnLightOff(bool sendStateToController) {
	digitalWrite(_relayPin, RELAY_OFF);
	saveState(lightId, STATE_OFF);
	_isLightOn = false;
	_isLightOnByPir = false;
	_pirGracePeriodStart = millis();
	if (sendStateToController) {
		sendLightStateToController();
	}
}

void SmartLight::sendLightStateToController() {
	MyMessage msg(lightId, V_LIGHT);
	send(msg.set(_isLightOn ? STATE_ON : STATE_OFF));
}

void SmartLight::changeLightState() {
	if (_isLightOn && _isLightOnByPir) {
		turnLightOn(false); //Controller already knows that light is turned on
	} else if (_isLightOn) {
		turnLightOff();
	} else {
		turnLightOn();
	}
}

void SmartLight::turnLightOnByPir() {
	if (!_isLightOn) {
		digitalWrite(_relayPin, RELAY_ON);
		_isLightOn = true;
		_isLightOnByPir = true;
		sendLightStateToController();
	}
}

void SmartLight::turnLightOffByPir() {
	if (_isLightOn && _isLightOnByPir) {
		digitalWrite(_relayPin, RELAY_OFF);
		_isLightOn = false;
		_isLightOnByPir = false;
		sendLightStateToController();
	}
}

void SmartLight::turnPirSwitchOn() {
    if(!_pirSwitch) {
        return;
    }
	saveState(pirSwitchId, STATE_ON);
	_isPirSwitchOn = true;
}

void SmartLight::turnPirSwitchOff() {
    if(!_pirSwitch) {
        return;
    }
	saveState(pirSwitchId, STATE_OFF);
	_isPirSwitchOn = false;
}

void SmartLight::sendPirSwitchStateToController() {
	MyMessage msg(pirSwitchId, V_ARMED);
	send(msg.set(_isPirSwitchOn ? STATE_ON : STATE_OFF));
}

bool SmartLight::isButtonActive() {
	return _debounce.update() && _debounce.read() == BUTTON_ACTIVE;
}

bool SmartLight::isPirActive() {
	if (_pirSensorPin == -1 || digitalRead(_pirSensorPin) != HIGH) {
		return false;
	}
	_pirActivatedTime = millis();
	return true;
}

bool SmartLight::hasPirSwitch() {
	return _pirSwitch;
}

bool SmartLight::shouldTurnLightOnByPir() {
    if (_isLightOnByPir || !_isPirSwitchOn) {
		return false;
	}
	unsigned long currentTime = millis();
	if (currentTime < _pirGracePeriodStart) { //Only TRUE when millis() will overflow (go back to zero), after approximately 50 days
		return currentTime > PIR_DETECTOR_GRACE_PERIOD;
	}
	return millis() - _pirGracePeriodStart > PIR_DETECTOR_GRACE_PERIOD;
}

bool SmartLight::shouldTurnLightOffByPir() {
	if (!_isLightOnByPir) {
		return false;
	}
	unsigned long currentTime = millis();
	if (currentTime < _pirActivatedTime) { //Only TRUE when millis() will overflow (go back to zero), after approximately 50 days
		return currentTime > PIR_DETECTOR_ACTIVE_TIME;
	}
	return millis() - _pirActivatedTime > PIR_DETECTOR_ACTIVE_TIME;
}

void SmartLight::initialize() {
	setLightInitialState();
	if(_pirSwitch) {
        setPirSwitchInitialState();
        sendPirSwitchStateToController();
    }
}

void SmartLight::setLightInitialState() {
	if (loadState(lightId) == STATE_ON) {
		turnLightOn();
	} else {
		turnLightOff();
	}
}

void SmartLight::setPirSwitchInitialState() {
	if (loadState(pirSwitchId) == STATE_ON) {
		turnPirSwitchOn();
	} else {
		turnPirSwitchOff();
	}
}

//=========SMART_DHT==========

SmartDHT::SmartDHT(int dhtPin): _dht(dhtPin, DHT11) {
  _dht.begin();
};

void SmartDHT::updateTemperature() {
    if(millis() - _lastUpdateTime < 3000) {
        return;
    }
    float temperature = _dht.readTemperature();
    MyMessage msg(id, V_TEMP);
    send(msg.set(temperature));
}

//=========SMART_HOME==========

SmartHome::SmartHome(SmartLight* smartLights, int collectionSize) {
	_smartLights = smartLights;
	_collectionSize = collectionSize;
	assignIds();
}

SmartHome::SmartHome(SmartLight* smartLights, int collectionSize, SmartDHT* smartDHTs, int collectionSize2) : SmartHome(smartLights, collectionSize) {
	_smartDHTs = smartDHTs;
    _collectionSize2 = collectionSize2;
}

void SmartHome::assignIds() {
	for (int i = 0; i < _collectionSize; ++i) {
		_smartLights[i].lightId = i + 1;
		if(_smartLights[i].hasPirSwitch()) {
		    _smartLights[i].pirSwitchId = i + 1 + _collectionSize;
		}
	}
	for (int i = 0; i < _collectionSize2; ++i) {
    	_smartDHTs[i].id = i + _collectionSize*2;
    }
}

void SmartHome::beSmart() {
    if(_isNotInitialized) {
        initialize();
    }
	for (int i = 0; i < _collectionSize; ++i) {
		if (_smartLights[i].isButtonActive()) {
			_smartLights[i].changeLightState();
		}
		if (_smartLights[i].isPirActive() && _smartLights[i].shouldTurnLightOnByPir()) {
			_smartLights[i].turnLightOnByPir();
		} else if (_smartLights[i].shouldTurnLightOffByPir()) {
			_smartLights[i].turnLightOffByPir();
		}
	}

	for (int i = 0; i < _collectionSize2; ++i) {
	    _smartDHTs[i].updateTemperature();
	}
}

void SmartHome::initialize() {
	for (int i = 0; i < _collectionSize; ++i) {
		_smartLights[i].initialize();
	}
	_isNotInitialized = false;
}

void SmartHome::doPresentation() {
	for (int i = 0; i < _collectionSize; ++i) {
		present(_smartLights[i].lightId, S_LIGHT);
		if(_smartLights[i].hasPirSwitch()) {
		    present(_smartLights[i].pirSwitchId, S_MOTION);
		}
	}
	for (int i = 0; i < _collectionSize2; ++i) {
        present(_smartDHTs[i].id, S_TEMP);
    }
}

void SmartHome::handleLightMessage(int lightId, int newState) {
	bool isOn = (bool)newState;
	if (isOn) {
		_smartLights[lightId].turnLightOn(false);
	} else {
		_smartLights[lightId].turnLightOff(false);
	}
}

void SmartHome::handlePirSwitchMessage(int pirSwitchId, int newState) {
	bool isOn = (bool)newState;
	if (isOn) {
		_smartLights[pirSwitchId-_collectionSize].turnPirSwitchOn();
	} else {
		_smartLights[pirSwitchId-_collectionSize].turnPirSwitchOff();
	}
}

//=========MY_SENSORS==========

void presentation() {
	sendSketchInfo("SmartHome", "1.0");
	smartHome.doPresentation();
}

void receive(const MyMessage &message) {
	if (message.type == V_LIGHT) {
		smartHome.handleLightMessage(message.sensor - 1, message.getInt());
	}
	if (message.type == V_ARMED) {
        smartHome.handlePirSwitchMessage(message.sensor - 1, message.getInt());
    }
}
