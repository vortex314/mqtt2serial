#include "ProtoThread.h"

//_______________________________________________________________________________________________________________
//
String Sys::cpu = "unknown";
String Sys::build = __DATE__ " " __TIME__;
String Sys::hostname = "Arduino";
String Sys::board = "unknown";

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
  LOG(" found %u  protothreads.",pts()->size());
  for (ProtoThread *pt : *pts()) {
    pt->setup();
  }
}

void ProtoThread::loopAll() {
  for (ProtoThread *pt : *pts()) {
  uint32_t startTime=Sys::millis();
    pt->loop();
    if ( (millis()-startTime)>10) {
      LOG(" slow protothread : %llu",Sys::millis()-startTime);
    }
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

//_______________________________________________________________________________________________________________
//
