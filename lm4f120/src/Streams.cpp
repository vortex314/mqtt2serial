
#include "Streams.h"


namespace std {
  void __throw_length_error(char const*) {
  while(1);
  }
  void __throw_bad_alloc(){
  while(1);
  }
  void __throw_bad_function_call()
{
  while (1)
    ;
}
}

//______________________________________________________________________________
//
Event::Event() : _word(0){};

Event::Event(int w) { _word = w; }
Event::Event(int src, int t)
{
  source = src;
  type = t;
}
bool Event::operator==(Event &second) { return _word == second._word; }

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
    	uint16_t newPos = (_readPtr + 1) % _size;

  if (newPos != _writePtr)
  {
    ret = _array[newPos];
    _readPtr=newPos;
    return true;
  }
    return false;
}
template <class T>
bool  CircularBuffer<T>::hasSpace() {
	return ((_writePtr + 1) % _size) != _readPtr;
}

template <class T>
void CircularBuffer<T>::push(T value)
{
    uint16_t newPos = (_writePtr + 1) % _size;

  if (newPos != _readPtr)
  {
    _array[_writePtr] = value;
    _writePtr=newPos;
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
void AbstractSource::operator>>(AbstractSink &sink) { addSink(&sink); }

void AbstractSource::operator>>(EventHandler handler){
  Invoker* invoker=new Invoker(handler);
  addSink(invoker);
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


Invoker::Invoker(EventHandler handler)
    : _handler(handler){};

void Invoker::recv(Event event){
  _handler(event);
}

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
    DISCONNECTED,
    PUBLISH
  };
  const Event Connected,Disconnected,Publish;
  Mqtt() : Connected(id(),CONNECTED),Disconnected(id(),DISCONNECTED),Publish(id(),PUBLISH) {
  }
  void recv(Event event){};
  void receiveLine(const String& line){
      send(Connected);
  };
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

class SerialPort:public Source{
  String _line;
  public:
  const Event RxdLine;
  SerialPort(): _line(""),RxdLine(id(),1){

  }
  const String& line() {
    return _line;
  }
};


class Receiver : public Sink {
  public:
  
};


void tester()
{
  Source source;
  Sink sink(100);
  Event evt;
  evt._word = (source.id() << 8) + 1;
  //source >> sink;
  SerialPort serial;
  Mqtt mqtt;
  Led ledBlue,ledGreen,ledRed;

  mqtt >> mqtt.Connected >> [&ledBlue](Event event){ ledBlue.blinkFast();};
  mqtt >> mqtt.Disconnected >> [&ledBlue](Event event){ ledBlue.blinkSlow();};
  serial >> serial.RxdLine >> [&mqtt,&serial](Event ev){mqtt.receiveLine(serial.line());};
  //mqtt >> mqtt.Publish >> [&mqtt](Event ev){ return mqtt.topic()=="dst/me/";} ;

}
