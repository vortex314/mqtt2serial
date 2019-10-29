#include <Arduino.h>

#include "proto.h"

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
#define PIN_LED PF_1

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
  void delay(uint32_t d) { _delay = d; }
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
#include <MedianFilter.h>
template <class T>
class MedianFilterFlow : public Flow<T, T>,public MedianFilter<T,10> {
public:
  MedianFilterFlow(uint32_t samples){};
  void recv(double d) { 
    this->addSample(d);
    if ( this->isReady()) this->emit(d); 
    };
};
//_______________________________________________________________________________________________________________
//
template <class T> class ToMqtt : public Flow<T, MqttMessage> {
  String _name;

public:
  ToMqtt(String name) : _name(name){};
  void recv(T event) {
    String s = "";
    DynamicJsonDocument doc(100);
    JsonVariant variant = doc.to<JsonVariant>();
    variant.set(event);
    serializeJson(doc, s);
    this->emit({_name, s});
    // emit doesn't work as such
    // https://stackoverflow.com/questions/9941987/there-are-no-arguments-that-depend-on-a-template-parameter
  }
//  static Flow<T, MqttMessage> &create(String s) { return *(new ToMqtt<T>(s)); }
};
//_______________________________________________________________________________________________________________
//
template <class T> class FromMqtt : public Flow<MqttMessage, T> {
  String _name;

public:
  FromMqtt(String name) : _name(name){};
  void recv(MqttMessage mqttMessage) {
    String s = "";
    DynamicJsonDocument doc(100);
    doc = mqttMessage;
    JsonVariant variant = doc.as<JsonVariant>();
    T value = variant.as<T>();
    this->emit(value);
    // emit doesn't work as such
    // https://stackoverflow.com/questions/9941987/there-are-no-arguments-that-depend-on-a-template-parameter
  }
//  static Flow<T, MqttMessage> &create(String s) { return *(new ToMqtt<T>(s)); }
};
//__________________________________________

MqttSerial mqtt(Serial);
LedBlinker ledBlinkerBlue(PIN_LED, 100);
Publisher publisher;
Tacho tacho(0);
Pwm pwm;

void setup() {
  Serial.begin(115200);
  Serial.println("\r\n===== Starting ProtoThreads  build " __DATE__
                 " " __TIME__);
  Sys::hostname = "stream2";
  Sys::cpu = "lm4f120h5qr";

  mqtt.connected >> ledBlinkerBlue.blinkSlow;
  mqtt >> [](MqttMessage m) {
    Serial.println(" Lambda :  RXD " + m.topic + "=" + m.message);
  };
  publisher >> mqtt;
  mqtt.connected >> new ToMqtt<bool>("mqtt/connected") >> mqtt;

  Source<double>& tachoFiltered = tacho >> new MedianFilterFlow<double>(10);
  tachoFiltered >>  pwm.rpmMeasured;
  tachoFiltered >> new ToMqtt<double>("tacho/rpm") >> mqtt;

  ProtoThread::setupAll();
}

void loop() { ProtoThread::loopAll(); }
