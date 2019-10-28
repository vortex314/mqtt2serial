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

public:
  Publisher(){};
  void setup() { LOG("Publisher started"); }
  void loop() {
    PT_BEGIN();
    while (true) {
      emit({"system/upTime", String(millis()), 1, false});
      emit({"system/build", "\""+Sys::build+"\"", 1, false});
      emit({"system/cpu", "\""+Sys::cpu+"\"", 1, false});
      emit({"system/heap", String(freeMemory()), 1, false});
      timeout(3000);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
};

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

class Pwm: public ProtoThread,public Source<MqttMessage>,public Sink<MqttMessage>{
  public:
    Sink<double> rpmMeasured;
    void setup(){};
    void loop(){};
    void recv(MqttMessage){};
};

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
    this->emit({_name, s, 0, false}); 
    // emit doesn't work  https://stackoverflow.com/questions/9941987/there-are-no-arguments-that-depend-on-a-template-parameter
  }
  static Flow<T,MqttMessage>& create(String s){
    return *(new ToMqtt<T>(s));
  }
};

//_____________________________________ protothreads running _____
//

MqttSerial mqtt(Serial);
LedBlinker ledBlinkerBlue(PIN_LED, 100);
Publisher publisher;
Tacho tacho(0);
ToMqtt<double> doubleToMqtt("tacho/rpm");
Pwm pwm;

void setup() {
  Serial.begin(115200);
  Serial.println("\r\n===== Starting ProtoThreads  build " __DATE__
                 " " __TIME__);
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
  tacho >> ToMqtt<double>::create("tacho/rpm") >> mqtt;
 // tacho >> pwm.rpmMeasured;
  ProtoThread::setupAll();
}

void loop() { ProtoThread::loopAll(); }
