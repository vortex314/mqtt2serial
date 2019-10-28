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
class Publisher : public ProtoThread, public Source<MqttMessage> {
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

class Tacho : public ProtoThread,public Source<double>{
  public:
    Tacho(uint32_t pwmIdx){};
    void setup(){};
    void loop(){};

};

template <class T> 
class ToMqtt : public Flow<T,MqttMessage> {
  String _name;
  public :
    ToMqtt(String name):_name(name){};
    void emit(MqttMessage m){
      
    }
    void recv(T event) {
      String s="";
      DynamicJsonDocument doc(100);
      JsonVariant variant = doc.to<JsonVariant>();
      variant.set(event);
      serializeJson(doc, s);
      MqttMessage m;
      emit(m);
      emit({_name,s,0,false});
    }
};

//_____________________________________ protothreads running _____
//

MqttSerial mqtt(Serial);
LedBlinker ledBlinkerBlue(PIN_LED, 100);
Publisher publisher;
Tacho tacho(0);

void setup() {
  Serial.begin(115200);
  LOG("===== Starting ProtoThreads  build " __DATE__ " " __TIME__);
  Sys::hostname = "stream2";
  Sys::cpu = "lm4f120h5qr";

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

  ToMqtt<double> toMqtt("tacho/rpm");
  tacho >> toMqtt >> mqtt;

  ProtoThread::setupAll();
}

void loop() { ProtoThread::loopAll(); }
