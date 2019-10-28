#include <Arduino.h>
#include <deque>
#include <functional>
#include <stdint.h>
#include <string>
#include <vector>

//______________________________________________________________________________
//
template <class T> class AbstractSink {
public:
  virtual void recv(T) = 0;
};
//______________________________________________________________________________
//
template <class T> class Sink : public AbstractSink<T> {
  std::function<void(T)> _handler;

public:
  Sink(){};
  Sink(std::function<void(T)> handler) : _handler(handler){};
  void handler(std::function<void(T)> handler) { _handler = handler; };
  void recv(T event) { _handler(event); };
};
//______________________________________________________________________________
//

template <typename T> class AbstractSource {
public:
  virtual void emit(T event) = 0;
};

template <class T> class Source : public AbstractSource<T> {
  std::vector<AbstractSink<T> *> _sinks;

public:
  Source(){};
  void addSink(AbstractSink<T> *_sink) { _sinks.push_back(_sink); }
  void operator>>(std::function<void(T)> handler) {
    addSink(new Sink<T>(handler));
  };
  void operator>>(Sink<T> &sink) { addSink(&sink); }
  void emit(T event) {
    for (AbstractSink<T> *sink : _sinks) {
      sink->recv(event);
    }
  };
};
//______________________________________________________________________________
//
template <class IN, class OUT>
class Flow : public AbstractSink<IN>, public Source<OUT> {};
//______________________________________________________________________________
//
template <class IN, class OUT>
Source<OUT> &operator>>(Source<IN> &source, Flow<IN, OUT> &flow) {
  source.addSink(&flow);
  return flow;
};
/*
template <class IN, class OUT>
void operator>>(Flow<IN, OUT> &flow, Sink<OUT> &sink) {
  flow.addSink(&sink);
};*/

//______________________________________________________________________________
//
template <class T> class BufferedSink : public Sink<T> {
  std::deque<T> _buffer;
  uint32_t _queueDepth;

public:
  BufferedSink(uint32_t size) : _queueDepth(size) {}
  void recv(T event) {
    if (_buffer.size() <= _queueDepth)
      _buffer.push_back(event);
    else
      Serial.println(__FILE__ ":" + String(__LINE__) + " | buffer full");
  };
  T getNext() {
    T t = _buffer.front();
    _buffer.pop_front();
    return t;
  }
  bool hasNext() { return !_buffer.empty(); }
};
//______________________________________________________________________________
//
typedef enum {
  CONNECTED,
  DISCONNECTED,
  OPEN,
  CLOSE,
  RXD,
  TXD,
  IO_ERROR
} Signal;