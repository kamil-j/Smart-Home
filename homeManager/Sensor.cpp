#include "Sensor.h"

Sensor::Sensor(int pin) {
	_pin = pin;
	homeManager.registerSensor(this);
}

Sensor::Sensor(int id, int pin) {
	_id = id;
	_pin = pin;
	homeManager.registerSensor(this);
}

int Sensor::getId() {
	return _id;
}