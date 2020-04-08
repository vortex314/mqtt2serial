#include <Arduino.h>
#include <MqttSerial.h>
#include <NanoAkka.h>

#include <deque>
#define PIN_LED 2
#define PIN_BUTTON 0

//_______________________________________________________________________________________________________________
//

//_______________________________________________________________________________________________________________
//

class LedBlinker : public Actor, public Sink<TimerMsg, 2> {
  uint32_t _pin;
  bool _on;

 public:
  Sink<bool, 2> blinkSlow;
  TimerSource blinkTimer;

 LedBlinker(Thread &thr, uint32_t pin, uint32_t delay)
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

  static void IRAM_ATTR isrButton() {
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

class Echo : public Actor {
  uint64_t _startTime;

 public:
  ValueSource<int> msgPerMsec = 0;
  ValueSource<int> out;
  Sink<int, 4> in;
  Echo(Thread &thr) : Actor(thr) {
    in.async(thread(), [&](const int &i) {
     INFO(" received %d ",i);
      out = i+1;
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
Echo echo(mainThread);

LambdaSource<uint32_t> systemHeap([]() { return ESP.getFreeHeap(); });
LambdaSource<uint64_t> systemUptime([]() { return Sys::millis(); });
LambdaSource<const char *> systemHostname([]() { return Sys::hostname(); });
LambdaSource<const char *> systemBoard([]() { return Sys::board(); });
LambdaSource<const char *> systemCpu([]() { return Sys::cpu(); });
ValueSource<const char *> systemBuild = __DATE__ " " __TIME__;
void serialEvent() {
  INFO("");
  MqttSerial::onRxd(&mqtt);
}
void setup() {
  Serial.begin(115200);
  Serial.println("\r\n===== Starting  build " __DATE__ " " __TIME__);
#ifndef HOSTNAME
  Sys::hostname("lm4f120");
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
  echo.out >> mqtt.toTopic<int>("system/echo");
  mqtt.fromTopic<int>("system/echo") >> echo.in;
  mqtt.incoming >> ([](const MqttMessage mm){
    INFO(" %s = %s",mm.topic.c_str(),mm.message.c_str());
  });
}

void loop() {
  mainThread.loop();
  if (Serial.available()) {
    MqttSerial::onRxd(&mqtt);
  }
}
