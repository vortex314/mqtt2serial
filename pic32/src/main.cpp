#include <Arduino.h>
#include <Arduino.h>

#include "ProtoThread.h"
#include <Streams.h>
#include <MqttSerial.h>
//_______________________________________________________________________________________________________________
//
#define PIN_LED PB1
#define PIN_BUTTON PB8
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
class LedBlinker : public ProtoThread {
  uint32_t _pin, _delay;

public:
  HandlerSink<bool> blinkSlow;
  LedBlinker(uint32_t pin, uint32_t delay) {
    _pin = pin;
    _delay = delay;
  }
  void setup() {
    LOG("LedBlinker started.");
    blinkSlow.handler([=](bool slow) {
      if (slow)
        _delay = 500;
      else
        _delay = 100;
    });
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
};
//_______________________________________________________________________________________________________________
//
class Button : public ProtoThread, public Source<bool> {
  uint32_t _pin;
  bool _pinOldValue;
  Timer _timer;
public:
  Button(uint32_t button) : _timer(1000,true,true) {
    if (button == 1)
      _pin = PIN_BUTTON;

  };
  void setup() { pinMode(_pin, INPUT_PULLUP); };
  void loop() {
    int pinNewValue;
    PT_BEGIN();
    while (true) {
      pinNewValue = digitalRead(_pin)==0;
      if ((_pinOldValue != pinNewValue) || _timer.timeout() ){
        emit(pinNewValue);
        _pinOldValue = pinNewValue;
        _timer.start();
      }
      timeout(10);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  };
};
//_______________________________________________________________________________________________________________
//
class Publisher : public ProtoThread, public Source<MqttMessage> {

public:
  Publisher(){};
  void setup() { LOG("Publisher started"); }
  void loop() {
    PT_BEGIN();
    while (true) {
      emit({"system/upTime", String(millis())});
      emit({"system/build", "\"" + Sys::build + "\""});
      emit({"system/cpu", "\"" + Sys::cpu + "\""});
      emit({"system/heap", String(freeMemory())});
      emit({"system/board","\"" + Sys::board + "\"" });
      timeout(3000);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
};
//_______________________________________________________________________________________________________________
//
class Tacho : public ProtoThread, public Source<double> {
public:
  Tacho(uint32_t pwmIdx){};
  void setup() { LOG("Tacho started"); };
  void loop() {
    PT_BEGIN();
    while (true) {
      emit(3.14);
      timeout(1000);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  };
};
//_______________________________________________________________________________________________________________
//
class Pwm : public ProtoThread,
            public Source<MqttMessage>,
            public AbstractSink<MqttMessage> {
public:
  HandlerSink<double> rpmMeasured;

  void setup() {
    rpmMeasured.handler([=](double rpm) {});
  };
  void loop(){};
  void recv(MqttMessage){};
};
//_______________________________________________________________________________________________________________
//
#include <MedianFilter.h>
template <class T>
class MedianFilterFlow : public Flow<T, T>, public MedianFilter<T, 10> {
public:
  MedianFilterFlow(uint32_t samples){};
  void recv(double d) {
    this->addSample(d);
    if (this->isReady())
      this->emit(d);
  };
};

//__________________________________________

MqttSerial mqtt(Serial);
LedBlinker ledBlinkerBlue(PIN_LED, 100);
Publisher publisher;
Tacho tacho(0);
Pwm pwm;
Button button1(1);
Button button2(2);

void setup() {
  Serial.begin(115200);
  Serial.println("\r\n===== Starting ProtoThreads  build " __DATE__
                 " " __TIME__);
  Sys::hostname = "pic32";
  Sys::cpu = "pic32mx250f128";
  Sys::board = "standalone";

  mqtt.connected >> ledBlinkerBlue.blinkSlow;

  mqtt >> [](MqttMessage m) {
    Serial.println(" Lambda :  RXD " + m.topic + "=" + m.message);
  };
  
  publisher >> mqtt;
  mqtt.connected >> new ToMqtt<bool>("mqtt/connected") >> mqtt;

  Source<double> &tachoFiltered = tacho >> new MedianFilterFlow<double>(10);
  tachoFiltered >> pwm.rpmMeasured;
  tachoFiltered >> new ToMqtt<double>("tacho/rpm") >> mqtt;

  button1>> new ToMqtt<bool>("button/button1") >> mqtt;

  mqtt >> new FromMqtt<double>("pwm/targetSpeed") >> new ToMqtt<double>("pwm/targetSpeed") >> mqtt;

  ProtoThread::setupAll();
}

void loop() { ProtoThread::loopAll(); }
