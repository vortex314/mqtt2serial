#ifndef STREAMS_H
#define STREAMS_H
#include <ArduinoJson.h>
#include <Log.h>
#include <atomic>
#include <deque>
#include <functional>
#include <string>
#include <vector>
//______________________________________________________________________________
//
template <class T> class Observer {
public:
  virtual void onNext(T) = 0;
};
template <class IN> class Sink : public Observer<IN> {};
//______________________________________________________________________________
//
// DD : used vector of void pointers and not vector of pointers to template
// class, to avoid explosion of vector implementations
class Requestable {
public:
  virtual void
  request() = 0; //{ WARN(" I am abstract Requestable. Don't call me."); };
};
// not sure these extra inheritance are useful
template <class T> class Source : public Requestable {
  std::vector<void *> _observers;

protected:
  uint32_t size() { return _observers.size(); }
  Observer<T> *operator[](uint32_t idx) {
    return static_cast<Observer<T> *>(_observers[idx]);
  }

public:
  void subscribe(Observer<T> &observer) {
    _observers.push_back((void *)&observer);
  }

  void emit(T t) {
    for (void *pv : _observers) {
      Observer<T> *pObserver = static_cast<Observer<T> *>(pv);
      pObserver->onNext(t);
    }
  }
};

// A flow can be both Sink and Source. Most of the time it will be in the middle
// of a stream
//______________________________________________________________________________________
//
template <class IN, class OUT>
class Flow : public Sink<IN>, public Source<OUT> {
public:
  Flow(){};
  Flow<IN, OUT>(Sink<IN> &a, Source<OUT> &b) : Sink<IN>(a), Source<OUT>(b){};
};
//_________________________________________ CompositeFlow
//_____________________________________________
//
template <class IN, class OUT> class CompositeFlow : public Flow<IN, OUT> {
  Sink<IN> &_in;
  Source<OUT> &_out;

public:
  CompositeFlow(Sink<IN> &a, Source<OUT> &b) : _in(a), _out(b){};
  void request() { _out.request(); };
  void onNext(IN in) { _in.onNext(in); }
};
//______________________________________________________________________________________
//
template <class IN, class INTERM, class OUT>
Flow<IN, OUT> &operator>>(Flow<IN, INTERM> &flow1, Flow<INTERM, OUT> &flow2) {
  flow1.subscribe(flow2);
  return *new CompositeFlow<IN, OUT>(flow1, flow2);
};

template <class IN, class OUT>
Sink<IN> &operator>>(Flow<IN, OUT> &flow, Sink<OUT> &sink) {
  flow.subscribe(sink);
  return flow;
};

template <class IN, class OUT>
Source<OUT> &operator>>(Source<IN> &source, Flow<IN, OUT> &flow) {
  source.subscribe(flow);
  return flow;
};

template <class OUT> void operator>>(Source<OUT> &source, Sink<OUT> &sink) {
  source.subscribe(sink);
};

//______________________________________________________________________________
//
//______________________________________________________________________________
//

template <class T> class ValueFlow : public Flow<T, T> {
  T _value;
  bool _emitOnChange = false;

public:
  ValueFlow() {}
  ValueFlow(T x) { _value = x; };
  void request() { this->emit(_value); }
  void onNext(T value) {
    if (_emitOnChange && (_value != value)) {
      this->emit(value);
    }
    _value = value;
  }
  void emitOnChange(bool b) { _emitOnChange = b; };
  inline void operator=(T value) { onNext(value); };
  inline T operator()() { return _value; }
};
//______________________________________________________________________________
//
template <class T> class LambdaSink : public Sink<T> {
  std::function<void(T)> _handler;

public:
  LambdaSink(){};
  LambdaSink(std::function<void(T)> handler) : _handler(handler){};
  void handler(std::function<void(T)> handler) { _handler = handler; };
  void onNext(T event) { _handler(event); };
};
//______________________________________________________________________________
//
template <class IN, class OUT> class LambdaFlow : public Flow<IN, OUT> {
  std::function<OUT(IN)> _handler;

public:
  LambdaFlow(){};
  LambdaFlow(std::function<OUT(IN)> handler) : _handler(handler){};
  void handler(std::function<OUT(IN)> handler) { _handler = handler; };
  void onNext(IN event) { _handler(event); };
};
//______________________________________________________________________________
//
template <class T> class Filter : public Flow<T, T> {
  T _value;

public:
  Filter(T value) { _value = value; }
  void onNext(T in) {
    if (in == _value)
      this->emit(in);
  }
};

#include <MedianFilter.h>

template <class T, int x> class Median : public Flow<T, T> {
  MedianFilter<T, x> _mf;

public:
  Median(){};
  void onNext(T value) {
    _mf.addSample(value);
    if (_mf.isReady()) {
      this->emit(_mf.getMedian());
    }
  };
  void request() { WARN(" not made for polling "); }
};

template <class T> class Router : public Flow<T, T> {
  uint32_t _idx;

public:
  void onNext(T t) {
    _idx++;
    if (_idx > this->size())
      _idx = 0;
    (*this)[_idx]->onNext(t);
  }
};
//______________________________________________________________________________
//
#ifdef FREERTOS
#include "freertos/task.h"
#include <FreeRTOS.h>
#include <freertos/semphr.h>

template <class T> class AsyncFlow : public Flow<T, T> {
  std::deque<T> _buffer;
  uint32_t _queueDepth;
  SemaphoreHandle_t xSemaphore = NULL;

public:
  AsyncFlow(uint32_t size) : _queueDepth(size) {
    xSemaphore = xSemaphoreCreateBinary();
    xSemaphoreGive(xSemaphore);
  }
  void onNext(T event) {
    if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
      if (_buffer.size() >= _queueDepth) {
        _buffer.pop_front();
        //					WARN(" buffer overflow in
        // BufferedSink
        //");
      }
      _buffer.push_back(event);
      xSemaphoreGive(xSemaphore);
      return;
    } else {
      WARN(" timeout on async buffer ! ");
    }
  }
  void onNextFromIsr(T event) {
    BaseType_t higherPriorityTaskWoken;
    if (xSemaphoreTakeFromISR(xSemaphore, &higherPriorityTaskWoken) == pdTRUE) {
      if (_buffer.size() >= _queueDepth) {
        _buffer.pop_front();
        //					WARN(" buffer overflow in
        // BufferedSink
        //");
      }
      _buffer.push_back(event);
      xSemaphoreGive(xSemaphore);
      return;
    } else {
      //           WARN(" timeout on async buffer ! "); // no log from ISR
    }
  }

  void request() {
    if (xSemaphoreTake(xSemaphore, (TickType_t)10) == pdTRUE) {
      T t;
      bool hasData = false;
      if (_buffer.size()) {
        t = _buffer.front();
        _buffer.pop_front();
        hasData = true;
      }
      xSemaphoreGive(xSemaphore);
      if (hasData)
        this->emit(t);
      return;
    } else {
      WARN(" timeout on async buffer ! ");
    }
  }
};
#else
#define STM32F1
#include <Sema.h>

template <class T> class AsyncFlow : public Flow<T, T> {
  std::deque<T> _buffer;
  uint32_t _queueDepth;
  Sema& _sema;

public:
  AsyncFlow(uint32_t size) : _queueDepth(size), _sema(Sema::create()) {}
  void onNext(T event) {
    _sema.wait();
    if (_buffer.size() >= _queueDepth) {
      _buffer.pop_front();
      //					WARN(" buffer overflow in
      // BufferedSink ");
    }
    _buffer.push_back(event);
    _sema.release();
    return;
  }
  void onNextFromIsr(T event) {
    _sema.wait();
    if (_buffer.size() >= _queueDepth) {
      _buffer.pop_front();
      //					WARN(" buffer overflow in
      // BufferedSink ");
    }
    _buffer.push_back(event);
    _sema.release();
    return;
  }

  void request() {
    _sema.wait();
    T t;
    bool hasData = false;
    if (_buffer.size()) {
      t = _buffer.front();
      _buffer.pop_front();
      hasData = true;
    }
    if (hasData)
      this->emit(t);
    _sema.release();
  }
};
#endif

template <class T> class AsyncValueFlow : Flow<T, T> {
  T _value;

public:
  void onNext(T value) { _value = value; }
  void request() { this->emit(_value); }
};

class AtomicSource : public Source<uint32_t> {
  std::atomic<uint32_t> _atom;

public:
  void inc() { _atom++; }
  void request() {
    if (_atom > 0) {
      emit(_atom);
      _atom--;
    }
  }
};

class TimerMsg {
public:
  uint32_t id;
};

class TimerSource : public Source<TimerMsg> {
  uint32_t _interval;
  bool _repeat;
  uint64_t _expireTime;
  uint32_t _id;

public:
  TimerSource(int id, uint32_t interval, bool repeat) {
    _id = id;
    _interval = interval;
    _repeat = repeat;
    _expireTime = Sys::millis() + _interval;
  }
  void interval(uint32_t i) { _interval = i; }
  void request() {
    if (Sys::millis() > _expireTime) {
      _expireTime = Sys::millis() + _interval;
      this->emit({_id});
    }
  }
};

#endif
