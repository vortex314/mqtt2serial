#include <Arduino.h>

#include <stdint.h>
#include <functional>
#include <string>
#include <vector>
//______________________________________________________________________________
//
//______________________________________________________________________________
//
template <class T>
class CircularBuffer {
  T *_array;
  uint32_t _writePtr;
  uint32_t _readPtr;
  uint32_t _size;

 public:
  CircularBuffer(uint32_t size) {
    _array = new T[size];
    _size = size;
    _writePtr = 1;
    _readPtr = 0;
  };
  bool empty() { return _readPtr == _writePtr; };
  bool hasSpace() { return ((_writePtr + 1) % _size) != _readPtr; }

  bool pop(T &ret) {
    uint16_t newPos = (_readPtr + 1) % _size;

    if (newPos != _writePtr) {
      ret = _array[newPos];
      _readPtr = newPos;
      return true;
    }
    return false;
  }
  void push(T value) {
    uint16_t newPos = (_writePtr + 1) % _size;

    if (newPos != _readPtr) {
      _array[_writePtr] = value;
      _writePtr = newPos;
    }
  }
};
//______________________________________________________________________________
//

//______________________________________________________________________________
//
template <class T>
class AbstractSink {
 public:
  virtual void recv(T) = 0;
};
//______________________________________________________________________________
//
template <class T>
class Sink : public AbstractSink<T> {
  std::function<void(T)> _handler;

 public:
  Sink(){};
  Sink(std::function<void(T)> handler) : _handler(handler){};
  void handler(std::function<void(T)> handler) { _handler = handler; };
  void recv(T event) { _handler(event); };
};
//______________________________________________________________________________
//

template <class T>
class AbstractSource {
  std::vector<AbstractSink<T> *> _sinks;

 public:
  AbstractSource(){};
  void addSink(AbstractSink<T> *_sink) {
    Serial.println(" added Sink");
    _sinks.push_back(_sink);
  }
  void operator>>(std::function<void(T)> handler) {
    addSink(new Sink<T>(handler));
  };
  void operator>>(AbstractSink<T> &sink) { addSink(&sink); }
  void emit(T event) {
    for (AbstractSink<T> *_sink : _sinks) {
      _sink->recv(event);
    }
  };
};

template <class T>
class BufferedSink : public AbstractSink<T> {
  CircularBuffer<T> _buffer;

 public:
  BufferedSink(uint32_t size) : _buffer(size) {}
  void recv(T event) { _buffer.push(event); };
  bool getNext(T &event) { return _buffer.pop(event); }
};
//______________________________________________________________________________
//
//______________________________________________________________________________
//
typedef enum { CONNECTED, DISCONNECTED } Signal;