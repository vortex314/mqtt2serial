#include <Arduino.h>

#include "proto.h"

//_______________________________________________________________________________________________________________
//

//_______________________________________________________________________________________________________________
//
#define PIN_LED PF_1

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
  void loop() {
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
class Publisher : public ProtoThread, public AbstractSource<MqttMessage> {
  String _systemPrefix;
 
public:
  Publisher(){};
  void setup() {
    LOG("Publisher started");
    _systemPrefix = "src/" + Sys::hostname + "/system/";
  }
  void loop() {
    PT_BEGIN();
    while (true) {
        emit({_systemPrefix + "upTime", String(millis()), 1, false});
        emit({_systemPrefix + "build", Sys::build, 1, false});
        emit({_systemPrefix + "cpu", Sys::cpu, 1, false});
        emit({_systemPrefix + "heap", String(freeMemory()), 1, false});
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
Publisher publisher;

void setup() {
  Serial.begin(115200);
  LOG("===== Starting ProtoThreads  build " __DATE__ " " __TIME__);
  Sys::hostname = "stream2";
  Sys::cpu = "lm4f120h5qr";
  //  mqtt.onMqttPublish(mqttCallback);
  mqtt.signalOut >> [](Signal s) {
    if (s == CONNECTED)
      ledBlinkerBlue.delay(500);
    else if (s == DISCONNECTED)
      ledBlinkerBlue.delay(100);
  };

  mqtt >> [](MqttMessage m) {
    Serial.println(" Lambda :  RXD " + m.topic + "=" + m.message);
  };

  publisher >> mqtt;

  ProtoThread::setupAll();
}

void loop() { ProtoThread::loopAll(); }
