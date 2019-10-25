
#include <Streams.h>
#include <functional>
#include <stdint.h>
#include <vector>

void std::__throw_bad_function_call()
{
  while (1)
    ;
}
/*
void std::__throw_length_error(char const*){
while(1);
}

void std::__throw_bad_alloc(char const*){
while(1);
}*/
//______________________________________________________________________________
//
Event::Event() : word(0){};

Event::Event(int w) { word = w; }
Event::Event(int src, int t)
{
  source = src;
  type = t;
}
bool Event::operator==(Event &second) { return word == second.word; }

//______________________________________________________________________________
//

template <class T>
CircularBuffer<T>::CircularBuffer(uint32_t size)
{
  _array = new T[size];
  _size = size;
  _writePtr = 1;
  _readPtr = 0;
};
template <class T>
bool CircularBuffer<T>::empty()
{
  return _readPtr == _writePtr;
};
template <class T>
bool CircularBuffer<T>::pop(T &ret)
{
  if (_readPtr != _writePtr)
  {
    ret = _array[_readPtr];
    if (++_readPtr == _size)
    {
      _readPtr = 0;
    }
    return true;
  }
  else
  {
    return false;
  }
}
template <class T>
void CircularBuffer<T>::push(T value)
{
  if (_writePtr != _readPtr)
  {
    _array[_writePtr] = value;
    if (++_writePtr == _size)
      _writePtr = 0;
  }
}

//______________________________________________________________________________
//
typedef std::function<void(Event &)> EventHandler;
//______________________________________________________________________________
//

//______________________________________________________________________________
//

AbstractSource::AbstractSource() { _id = _sourceIndex++; }

void AbstractSource::addSink(AbstractSink *_sink) { _sinks.push_back(_sink); }
void AbstractSource::operator>>(AbstractSink &sink) { _sinks.push_back(&sink); }

void AbstractSource::operator>>(EventHandler handler){

};

AbstractSource &AbstractSource::operator>>(Event event)
{
  Filter *filter = new Filter(event);
  return *filter;
};

AbstractSource &AbstractSource::operator>>(AbstractFlow &flow)
{
  addSink(&flow);
  return flow;
}

void AbstractSource::send(Event event)
{
  for (AbstractSink *_sink : _sinks)
  {
    _sink->recv(event);
  }
}
uint8_t AbstractSource::id() { return _id; };
void AbstractSource::id(uint8_t id) { _id = id; };

//______________________________________________________________________________
//

//______________________________________________________________________________
//

Sink::Sink(uint32_t size) : _buffer(size){};
Sink::Sink(uint32_t size, EventHandler handler) : _buffer(size){};
void Sink::recv(Event event) { _buffer.push(event); };
bool Sink::getNext(Event &event) { return _buffer.pop(event); };

//______________________________________________________________________________
//

Filter::Filter(Event event) { _match = event; };
void Filter::recv(Event event)
{
  if (event == _match)
    send(event);
}

//______________________________________________________________________________
//

void Invoker::send(Event &event)
{
  if (_event.word == event.word)
    _handler(event);
}
Invoker::Invoker(Event event, EventHandler handler)
    : _event(event), _handler(handler){};

Source::Source(){};
void on(Event event, EventHandler handler) {}
AbstractFlow &Source::filter(int type)
{
  AbstractFlow *flow = new Filter(Event(this->id(), type));
  operator>>(*flow);
  return *flow;
}
void Source::invoke(Event event, EventHandler handler) {}

uint8_t AbstractSource::_sourceIndex = 1;

class Mqtt : public AbstractFlow
{
public:
  enum
  {
    CONNECTED,
    DISCONNECTED
  };
  const Event Connected,Disconnected;
  Mqtt() : Connected(id(),CONNECTED),Disconnected(id(),DISCONNECTED) {
  }
  void recv(Event event){};
};

class Led : public Sink
{
public:
  Led() : Sink(10){};
  enum
  {
    BLINK_FAST,
    BLINK_SLOW
  };
  void recv(Event event){};

  void blinkFast(){};
  void blinkSlow(){};
};

class On : public AbstractFlow
{
  int _type;

public:
  On(int type) { _type = type; };
  static AbstractFlow &type(int type) { return *new On(type); }
  void recv(Event event)
  {
    if (event.type == _type)
      send(event);
  };
};

class InvokeDirect : public Sink
{
  EventHandler _handler;

public:
  InvokeDirect(EventHandler handler) : Sink(0) { _handler = handler; };
  void recv(Event event) { _handler(event); }
  static InvokeDirect &call(EventHandler handler)
  {
    return *new InvokeDirect(handler);
  }
};

class MyEvent
{
  uint16_t _from;
  uint16_t _type;
  void *_data;
};

void tester()
{
  Source source;
  Sink sink(100);
  Event evt;
  evt.word = (source.id() << 8) + 1;
  //source >> sink;
  Source serial;
  Mqtt mqtt;
  Led led;

  mqtt >> mqtt.Connected >> [&led](Event event){ led.blinkFast();};
  mqtt >> mqtt.Disconnected >> [&led](Event event){ led.blinkSlow();};
  serial >> mqtt;

  //Sink blinkFast(1, [&led](Event event) { led.blinkFast(); });

  /*   mqtt.map(Mqtt::CONNECTED, Led::BLINK_SLOW) >> led;
     mqtt.map(Mqtt::DISCONNECTED, Led::BLINK_FAST) >> led;
     serial >> mqtt;

     mqtt.on(Mqtt::DISCONNECTED) >> [](Event event){};

     mqtt >> On::type(Mqtt::DISCONNECTED) >> InvokeDirect::call([&led](Event ev)
     { led.blinkFast(); }); mqtt >> On::type(Mqtt::CONNECTED) >>
     InvokeDirect::call([&led](Event ev) { led.blinkSlow(); });*/
}