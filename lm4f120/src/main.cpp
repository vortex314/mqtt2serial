#include <Arduino.h>

#include "ProtoThread.h"
#include <MqttSerial.h>
#include <deque>
#include <stdio.h>

#define PIN_LED PF_1

//_______________________________________________________________________________________________________________
//
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);
#else  // __ARM__
extern char *__brkval;
#endif // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char *>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else  // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif // __arm__
}
//_______________________________________________________________________________________________________________
//

class LedBlinker : public Sink<TimerMsg> {
  uint32_t _pin;
  bool _on;

public:
  LambdaSink<bool> blinkSlow;
  TimerSource blinkTimer;

  LedBlinker(uint32_t pin, uint32_t delay);
  void setup();
  void delay(uint32_t d);
  void onNext(TimerMsg);
};

LedBlinker::LedBlinker(uint32_t pin, uint32_t delay)
    : blinkTimer(1, delay, true) {
  _pin = pin;
  blinkTimer.interval(delay);
  blinkSlow.handler([=](bool flag) {
    if (flag)
      blinkTimer.interval(500);
    else
      blinkTimer.interval(100);
  });
}
void LedBlinker::setup() {
  pinMode(_pin, OUTPUT);
  digitalWrite(_pin, 1);
  blinkTimer >> *this;
}
void LedBlinker::onNext(TimerMsg m) {
  digitalWrite(_pin, _on);
  _on = !_on;
}
void LedBlinker::delay(uint32_t d) { blinkTimer.interval(d); }

//_______________________________________________________________________________________________________________
//
class Button : public Source<bool>, public Sink<TimerMsg> {
  uint32_t _pin;
  bool _pinOldValue;

public:
  Button(uint32_t button) {
    if (button == 1)
      _pin = PF_4;
    if (button == 2)
      _pin = PF_0;
  };
  void init() { pinMode(_pin, INPUT_PULLUP); };
  void onNext(TimerMsg m) { request(); }
  void request() {
    int pinNewValue;
    pinNewValue = digitalRead(_pin) == 0;
    if ((_pinOldValue != pinNewValue)) {
      emit(pinNewValue);
      _pinOldValue = pinNewValue;
    }
  }
};
//_______________________________________________________________________________________________________________
//
class Publisher : public Sink<TimerMsg>, public Source<MqttMessage> {

public:
  Publisher(){};

  void onNext(TimerMsg m) { request(); }
  void request() {
    emit({"system/upTime", String(millis())});
    emit({"system/build", "\"" + Sys::build + "\""});
    emit({"system/cpu", "\"" + Sys::cpu + "\""});
    emit({"system/heap", String(freeMemory())});
    emit({"system/board", "\"" + Sys::board + "\""});
  }
};

//_______________________________________________________________________________________________________________
//
//_______________________________________________________________________________________________________________
//
//__________________________________________

MqttSerial mqtt(Serial);
LedBlinker ledBlinkerBlue(PIN_LED, 100);
Publisher publisher;
Button button1(1);
Button button2(2);
TimerSource timerButton(10, true, true);
TimerSource timerLed(100, true, true);

void setup() {
  Serial.begin(115200);
  Serial.println("\r\n===== Starting ProtoThreads  build " __DATE__
                 " " __TIME__);
  Sys::hostname = "stream2";
  Sys::cpu = "lm4f120h5qr";

  mqtt.connected >> ledBlinkerBlue.blinkSlow;
  publisher >> mqtt.outgoing;
  mqtt.connected >> mqtt.toTopic<bool>("mqtt/connected");

  button1 >> mqtt.toTopic<bool>("button/button1");
  button2 >> mqtt.toTopic<bool>("button/button2");

  mqtt.fromTopic<double>("pwm/targetSpeed") >>
      mqtt.toTopic<double>("pwm/targetSpeed");

}

void loop() {
  mqtt.request();
  ledBlinkerBlue.blinkTimer.request();
  publisher.request();
  button1.request();
  button2.request();
}
