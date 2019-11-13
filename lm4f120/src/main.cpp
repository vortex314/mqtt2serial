#include <Arduino.h>
#include <MqttSerial.h>
#include <deque>
//#include <stdio.h>

#define PIN_LED PF_2

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
  void init();
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
void LedBlinker::init() {
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
class Button : public AsyncFlow<bool> {
  uint32_t _pin;
  bool _pinOldValue;
  static Button *_me;

public:
  Button(uint32_t button) : AsyncFlow<bool>(20) {
    if (button == 1)
      _pin = PF_4;
    if (button == 2)
      _pin = PF_0;
    _me = this;
  };

  static void ISR() { _me->onNext(digitalRead(_me->_pin) == 0); LOG(""); }

  void init() {
    pinMode(_pin, INPUT_PULLUP);
    attachInterrupt(_pin, ISR, CHANGE);
  };
};

Button *Button::_me;

//_______________________________________________________________________________________________________________
//
//______________________________________________________________________
//
class Poller : public Sink<TimerMsg> {
  std::vector<Requestable *> _requestables;
  uint32_t _idx = 0;

public:
  ValueFlow<bool> run = false;
  Poller(){};
  void onNext(TimerMsg m) {
    _idx++;
    if (_idx >= _requestables.size())
      _idx = 0;
    if (_requestables.size() && run()) {
      _requestables[_idx]->request();
    }
  }
  Poller &operator()(Requestable &rq) {
    _requestables.push_back(&rq);
    return *this;
  }
};
//_______________________________________________________________________________________________________________
//
//__________________________________________

MqttSerial mqtt(Serial);
LedBlinker ledBlinkerBlue(PIN_LED, 100);
Button button1(1);
Button button2(2);
TimerSource timerButton(1, 10, true);
TimerSource timerLed(1, 100, true);
TimerSource ticker(1, 1, true);
TimerSource pollTimer(1, 1000, true);

Poller poller;

ValueFlow<String> systemBuild;
ValueFlow<String> systemHostname;
ValueFlow<String> systemCpu;
ValueFlow<String> systemBoard;

// ValueFlow<bool> systemAlive=true;
LambdaSource<uint32_t> systemHeap([]() { return freeMemory(); });
LambdaSource<uint64_t> systemUptime([]() { return Sys::millis(); });

void setup() {
  Serial.begin(115200);
  Serial.println("\r\n===== Starting  build " __DATE__ " " __TIME__);
  Sys::hostname = "lm4f120";
  Sys::cpu = "lm4f120h5qr";
  Sys::board = "stellaris";
  systemBuild = __DATE__ " " __TIME__;
  systemHostname = Sys::hostname;
  systemCpu = Sys::cpu;
  systemBoard = Sys::board;
  button1.init();
  button2.init();
  ledBlinkerBlue.init();

  mqtt.connected >> ledBlinkerBlue.blinkSlow;
  mqtt.connected >> poller.run;
  mqtt.connected >> mqtt.toTopic<bool>("mqtt/connected");

  systemHeap >> mqtt.toTopic<uint32_t>("system/heap");
  systemUptime >> mqtt.toTopic<uint64_t>("system/upTime");
  systemBuild >> mqtt.toTopic<String>("system/build");
  systemHostname >> mqtt.toTopic<String>("system/hostname");
  systemBoard >> mqtt.toTopic<String>("system/board");

  pollTimer >> poller(systemHostname)(systemHeap)(systemBuild)(systemUptime)(
                   systemBoard);

  button1 >> mqtt.toTopic<bool>("button/button1");
  button2 >> mqtt.toTopic<bool>("button/button2");
  auto& logger = *new LambdaSink<bool>([](bool b) { LOG("BUTTON %d", b); });
  button1 >> logger;
  button2 >> logger;

  mqtt.init();
}

void loop() {
  mqtt.request();
  ledBlinkerBlue.blinkTimer.request();
  timerLed.request();
  pollTimer.request();
  mqtt.outgoing.request();
  button1.request();
  button2.request();
}
