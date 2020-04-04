#include <Arduino.h>
#include <MqttSerial.h>
#include <NanoAkka.h>

#include <deque>
//#include <stdio.h>
namespace std {
void __throw_bad_function_call() {
  WARN("invalid function called");
  while (1)
    ;
}
}  // namespace std
#define PIN_LED 20
#define PIN_BUTTON 17

//_______________________________________________________________________________________________________________
//
#ifdef __arm__
// should use uinstd.h to define sbrk but Due causes a conflict
extern "C" char *sbrk(int incr);
#else   // __ARM__
extern char *__brkval;
#endif  // __arm__

int freeMemory() {
  char top;
#ifdef __arm__
  return &top - reinterpret_cast<char *>(sbrk(0));
#elif defined(CORE_TEENSY) || (ARDUINO > 103 && ARDUINO != 151)
  return &top - __brkval;
#else   // __arm__
  return __brkval ? &top - __brkval : &top - __malloc_heap_start;
#endif  // __arm__
}
//_______________________________________________________________________________________________________________
//

class LedBlinker : public Actor, public Sink<TimerMsg, 2> {
  uint32_t _pin;
  bool _on;

 public:
  Sink<bool, 2> blinkSlow;
  TimerSource blinkTimer;

  LedBlinker(Thread &thr, uint32_t pin, uint32_t delay);
  void init() {
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, 1);
    blinkTimer >> *this;
  };
  void delay(uint32_t d) { blinkTimer.interval(d); };
  void on(const TimerMsg &) {
    digitalWrite(_pin, _on);
    _on = !_on;
  };
};

LedBlinker::LedBlinker(Thread &thr, uint32_t pin, uint32_t delay)
    : Actor(thr), blinkTimer(thr, 1, delay, true) {
  _pin = pin;
  blinkTimer.interval(delay);
  blinkSlow.sync([&](bool flag) {
    if (flag)
      blinkTimer.interval(500);
    else
      blinkTimer.interval(100);
  });
}

//_______________________________________________________________________________________________________________
//
class Button : public Actor, public ValueFlow<bool> {
  uint32_t _pin;
  bool _pinOldValue;
  static Button *_button;
  static Button *_button2;
  bool _lastState = false;

 public:
  Button(Thread &thr, uint32_t pin) : Actor(thr), ValueFlow<bool>(), _pin(pin) {
    _button = this;
  };

  void newValue(bool b) {
    if (b != _lastState) {
      _lastState = b;
      on(b);
    }
  }

  static void isrButton() {
    if (_button) _button->newValue(digitalRead(_button->_pin) == 0);
  }
  void init() {
    pinMode(_pin, INPUT_PULLUP);
    attachInterrupt(_pin, isrButton, CHANGE);
  };
};

Button *Button::_button = 0;
//______________________________________________________________________
//
class Pinger : public Actor {
  int _counter = 0;

 public:
  ValueSource<int> out;
  Sink<int, 4> in;
  Pinger(Thread &thr) : Actor(thr) {
    in.async(thread(), [&](const int &i) { out = _counter++; });
  }
  void start() { out = _counter++; }
};
#define DELTA 50000
class Echo : public Actor {
  uint64_t _startTime;

 public:
  ValueSource<int> msgPerMsec = 0;
  ValueSource<int> out;
  Sink<int, 4> in;
  Echo(Thread &thr) : Actor(thr) {
    in.async(thread(), [&](const int &i) {
      //      INFO("");
      if (i % DELTA == 0) {
        uint64_t endTime = Sys::millis();
        uint32_t delta = endTime - _startTime;
        msgPerMsec = DELTA / delta;
        INFO(" handled %d messages in %lu msec = %d msg/msec ", DELTA, delta,
             msgPerMsec());
        _startTime = Sys::millis();
      }
      out = i;
    });
  }
};

class Poller : public Actor, public Sink<TimerMsg, 2> {
  TimerSource _pollInterval;
  std::vector<Requestable *> _publishers;
  uint32_t _idx = 0;
  bool _connected;

 public:
  Sink<bool, 2> connected;
  Poller(Thread &thr) : Actor(thr), _pollInterval(thr, 1, 100, true) {
    _pollInterval >> this;
    connected.async(thread(), [&](const bool &b) { _connected = b; });
    async(thread(), [&](const TimerMsg tm) {
      if (_publishers.size() && _connected)
        _publishers[_idx++ % _publishers.size()]->request();
    });
  };
  void setInterval(uint32_t t) { _pollInterval.interval(t); }
  Poller &operator()(Requestable &rq) {
    _publishers.push_back(&rq);
    return *this;
  }
};
//_______________________________________________________________________________________________________________
//
//__________________________________________

// MqttSerial mqtt(mainThread,Serial);
Thread mainThread("main");
LedBlinker ledBlinkerBlue(mainThread, PIN_LED, 100);
Button button1(mainThread, PIN_BUTTON);
Poller poller(mainThread);
MqttSerial mqtt(mainThread);
Pinger pinger(mainThread);
Echo echo(mainThread);

LambdaSource<uint32_t> systemHeap([]() { return freeMemory(); });
LambdaSource<uint64_t> systemUptime([]() { return Sys::millis(); });
LambdaSource<const char *> systemHostname([]() { return Sys::hostname(); });
LambdaSource<const char *> systemBoard([]() { return Sys::board(); });
LambdaSource<const char *> systemCpu([]() { return Sys::cpu(); });
ValueSource<const char *> systemBuild = __DATE__ " " __TIME__;
// void serialEvent() { MqttSerial::onRxd(&mqtt); }
void setup() {
  Serial.begin(115200);
  Serial.println("\r\n===== Starting  build " __DATE__ " " __TIME__);
#ifndef HOSTNAME
  Sys::hostname("unknown");
#else
  Sys::hostname(S(HOSTNAME));
#endif
  button1.init();
  ledBlinkerBlue.init();
  mqtt.init();

  mqtt.connected >> ledBlinkerBlue.blinkSlow;
  mqtt.connected >> poller.connected;
  mqtt.connected >> mqtt.toTopic<bool>("mqtt/connected");

  systemHeap >> mqtt.toTopic<uint32_t>("system/heap");
  systemUptime >> mqtt.toTopic<uint64_t>("system/upTime");
  systemBuild >> mqtt.toTopic<const char *>("system/build");
  systemHostname >> mqtt.toTopic<const char *>("system/hostname");
  systemBoard >> mqtt.toTopic<const char *>("system/board");
  systemCpu >> mqtt.toTopic<const char *>("system/cpu");

  systemBoard >> ([](const char *board) { INFO("board : %s ", board); });
  poller.connected.on(true);
  poller(systemHostname)(systemHeap)(systemBuild)(systemUptime)(systemBoard)(
      systemCpu);

  button1 >> mqtt.toTopic<bool>("button/button1");
  poller(button1);
  pinger.out >> echo.in;  // the wiring
  echo.out >> pinger.in;
  //  pinger.start();
  Serial.println(" sizeof(int) : " + String(sizeof(int)));
}

void loop() {
  mainThread.loop();
  if (Serial.available()) {
    MqttSerial::onRxd(&mqtt);
  }
}
