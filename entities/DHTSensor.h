#ifndef SensorDHT_h
#define SensorDHT_h

#include <DHT.h>

class DHTSensor: public Entity {
public:
    DHTSensor(int id, int pin): Entity(id, V_TEMP, pin), _dht(pin, DHT11) {
        _dht.begin();
    }

    void presentation() {
        present(_id, S_TEMP);
    }

    void onLoop() {
        if(millis() - _lastUpdateTime > DHT_UPDATE_TIME) {
            updateTemperature();
        }
    }

private:
    DHT _dht;
    unsigned long _lastUpdateTime = 0;

    void updateTemperature() {
        float temperature = _dht.readTemperature();
        send(_msg.set(temperature, 1));
        _lastUpdateTime = millis();
    }
};

#endif