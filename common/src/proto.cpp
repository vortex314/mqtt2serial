#include "proto.h"

//_______________________________________________________________________________________________________________
//
String Sys::cpu = "unknown";
String Sys::build = __DATE__ " " __TIME__;
String Sys::hostname = "Arduino";

Timer::Timer(uint32_t delta, bool repeat, bool actif) {
  _delta = delta;
  _repeat = repeat;
  _actif = actif;
}

bool Timer::isRepeating() { return _repeat; }
void Timer::repeat(bool rep) { _repeat = rep; }
void Timer::start() {
  _timeout = millis() + _delta;
  _actif = true;
}

void Timer::stop() { _actif = false; }

bool Timer::timeout() {
  if (_actif)
    return millis() > _timeout;
  return false;
}

void Timer::timeout(uint32_t delay) {
  _timeout = millis() + delay;
  _actif = true;
}

//_______________________________________________________________________________________________________________
//
// to avoid the problem that static objects are created in disorder, it is
// created when used.
//
std::vector<ProtoThread *> *ProtoThread::_pts;

std::vector<ProtoThread *> *ProtoThread::pts() {
  if (ProtoThread::_pts == 0) {
    ProtoThread::_pts = new std::vector<ProtoThread *>();
  }
  return ProtoThread::_pts;
}

ProtoThread::ProtoThread() : _defaultTimer(1, false, false), _ptLine(0) {
  //      LOG(" new protoThread");
  _bits = 0;
  pts()->push_back(this);
}

ProtoThread::~ProtoThread() {}
bool ProtoThread::timeout() { return _defaultTimer.timeout(); }
void ProtoThread::timeout(uint32_t delay) {
  if (delay == 0)
    _defaultTimer.stop();
  else {
    _defaultTimer.timeout(delay);
  }
}

void ProtoThread::setupAll() {
  LOG(" found " + String(pts()->size()) + " protothreads.");
  for (ProtoThread *pt : *pts()) {
    pt->setup();
  }
}

void ProtoThread::loopAll() {
  for (ProtoThread *pt : *pts()) {
    pt->loop();
  }
}
void ProtoThread::restart() { _ptLine = 0; }
void ProtoThread::stop() { _ptLine = LineNumberInvalid; }
bool ProtoThread::isRunning() { return _ptLine != LineNumberInvalid; }
bool ProtoThread::isReady() { return _ptLine == LineNumberInvalid; }

bool ProtoThread::setBits(uint32_t bits) {
  uint32_t expected = _bits;
  uint32_t desired = _bits | bits;
  return _bits.compare_exchange_strong(expected, desired);
}

bool ProtoThread::clrBits(uint32_t bits) {
  uint32_t expected = _bits;
  uint32_t desired = _bits & bits;
  return _bits.compare_exchange_strong(expected, desired);
}

bool ProtoThread::hasBits(uint32_t bits) { return (_bits & bits); }
//_______________________________________________________________________________________________________________
//

MqttSerial::MqttSerial(Stream &stream)
    : AbstractSink<MqttMessage>(), _stream(stream),
      _loopbackTimer(1000, true, true), _connectTimer(3000, true, true) {}

MqttSerial::~MqttSerial() {}

void MqttSerial::recv(MqttMessage m) {
  if (_connected)
    publish(m.topic, m.message);
}

void MqttSerial::setup() {
  //  LOG(__FUNCTION__);
  txd.clear();
  rxd.clear();
  _loopbackTopic = "dst/" + Sys::hostname + "/system/loopback";
  _loopbackTimer.start();
  _connectTimer.start();
  _loopbackReceived = 0;
}

void MqttSerial::loop() {
  PT_BEGIN();
  timeout(1000);
  while (true) {
    PT_YIELD_UNTIL(_stream.available() || timeout() ||
                   _loopbackTimer.timeout());
    if (_stream.available()) {
      rxdSerial(_stream.readString());
    };
    if (_connectTimer.timeout()) {
      if (millis() > (_loopbackReceived + 2000)) {
        _connected = false;
        signalOut.emit(DISCONNECTED);
        subscribe("dst/" + Sys::hostname + "/#");
        publish(_loopbackTopic, "true");
      } else {
        _connected = true;
        signalOut.emit(CONNECTED);
      }
      _connectTimer.start();
    }
    if (_loopbackTimer.timeout()) {
      publish(_loopbackTopic, "true");
      _loopbackTimer.start();
    }
  }
  PT_END();
}

void MqttSerial::rxdSerial(String s) {
  for (uint32_t i = 0; i < s.length(); i++) {
    char c = s.charAt(i);
    if ((c == '\n' || c == '\r')) {
      if (rxdString.length() > 0) {
        deserializeJson(rxd, rxdString);
        JsonArray array = rxd.as<JsonArray>();
        if (!array.isNull()) {
          if (array[1].as<String>() == _loopbackTopic) {
            _loopbackReceived = millis();
            published.emit({array[1], array[2], 0, false});
          } else {
            published.emit({array[1], array[2], 0, false});
            //            _callback(array[1].as<String>(),
            //            array[2].as<String>());
          }
        }
        rxdString = "";
      }
    } else {
      if (rxdString.length() < 256)
        rxdString += c;
    }
  }
}

void MqttSerial::publish(String topic, String message) {
  txd.clear();
  txd.add((int)CMD_PUBLISH);
  txd.add(topic);
  txd.add(message);
  sendSerial();
}

void MqttSerial::subscribe(String topic) {
  txd.clear();
  txd.add((int)CMD_SUBSCRIBE);
  txd.add(topic);
  sendSerial();
}

void MqttSerial::sendSerial() {
  String output = "";
  serializeJson(txd, output);
  _stream.println(output);
  _stream.flush();
}
bool MqttSerial::isConnected() { return _connected; }

//_______________________________________________________________________________________________________________
//
