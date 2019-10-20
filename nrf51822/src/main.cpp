#include <Arduino.h>

#include "proto.h"

//_______________________________________________________________________________________________________________
//

//_______________________________________________________________________________________________________________
//
#define PIN_LED 20

class LedBlinker : public ProtoThread {
  uint32_t _pin, _delay;

public:
  LedBlinker(uint32_t pin, uint32_t delay) {
    _pin = pin;
    _delay = delay;
  }
  void setup() {
    LOG("LedBlinker started.");
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, 1);
  }
  bool loop() {
    PT_BEGIN();
    while (true) {
      timeout(_delay);
      digitalWrite(_pin, 0);
      PT_YIELD_UNTIL(timeout());
      timeout(_delay);
      digitalWrite(_pin, 1);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
  void delay(uint32_t d) { _delay = d; }
};
//_______________________________________________________________________________________________________________
//
// LedBlinker ledBlinkerRed(LED_RED_PIN,100);
// LedBlinker ledBlinkerGreen(LED_GREEN_PIN,300);
//_______________________________________________________________________________________________________________
//
class Publisher : public ProtoThread {
  String _systemPrefix;
  MqttSerial &_mqtt;
  LedBlinker &_ledBlinker;

public:
  Publisher(MqttSerial &mqtt, LedBlinker &ledBlinker)
      : _mqtt(mqtt), _ledBlinker(ledBlinker){};
  void setup() {
    LOG("Publisher started");
    _systemPrefix = "src/" + Sys::hostname + "/system/";
  }
  bool loop() {
    PT_BEGIN();
    while (true) {
      if (_mqtt.isConnected()) {
        _mqtt.publish(_systemPrefix + "upTime", String(millis()));
        _mqtt.publish(_systemPrefix + "build", Sys::build);
        _mqtt.publish(_systemPrefix + "cpu", Sys::cpu);
        _mqtt.publish(_systemPrefix + "heap", String(freeMemory()));
        _ledBlinker.delay(1000);
      } else
        _ledBlinker.delay(100);
      timeout(1000);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
};
//
//_____________________________________ protothreads running _____
//

MqttSerial mqtt(Serial);
LedBlinker ledBlinkerBlue(PIN_LED, 100);
Publisher publisher(mqtt, ledBlinkerBlue);

void mqttCallback(String topic, String message) {
  Serial.println(" RXD " + topic + "=" + message);
}

void setup() {
  Serial.begin(115200);
  LOG("===== Starting ProtoThreads  build " __DATE__ " " __TIME__);
  Sys::hostname = "stellaris";
  Sys::cpu = "lm4f120h5qr";
  mqtt.onMqttPublish(mqttCallback);
  ProtoThread::setupAll();
}

void loop() { ProtoThread::loopAll(); }
