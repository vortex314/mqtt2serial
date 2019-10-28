#include <Arduino.h>

#include <stdint.h>
#include <functional>
#include <string>
#include <vector>
#include <deque>
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

template <typename T> 
class AbstractSource {
public:
virtual void emit(T event)=0;
};
/*
template <class I,class O> 
class AbstractFlow : public Sink<I>,public AbstractSource<O> {
} ;*/

template <class T>
class Source : public AbstractSource<T> {
  std::vector<AbstractSink<T> *> _sinks;

 public:
  Source(){};
  void addSink(AbstractSink<T> *_sink) {
    _sinks.push_back(_sink);
  }
  void operator>>(std::function<void(T)> handler) {
    addSink(new Sink<T>(handler));
  };
  void operator>>(Sink<T> &sink) { addSink(&sink); }
  void emit(T event) {
    for (AbstractSink<T> *sink : _sinks) {
      sink->recv(event);
    }
  };
 // template <class X>
//  Source<X> operator>>(AbstractFlow<T,X> & flow ) { addSink(&flow); return flow;}
};

template <class I,class O> 
class Flow : public Sink<I>,public Source<O> {

};

template <class IN, class OUT>
Source<OUT>&  operator>>(Source<IN> &source,Flow<IN,OUT> &flow){
source.addSink(&flow);
return flow;
};
/*
template <class T>
void operator>>(Source<T> &source,Sink<T> &sink){
source.addSink(&sink);
};*/
/*
template <class T>
  void operator>>(Source<T>&  source,std::function<void(T)> handler) {
    source.addSink(new Sink<T>(handler));
  };*/

template <class T>
class BufferedSink : public Sink<T> {
  std::deque<T> _buffer;

 public:
  BufferedSink(uint32_t size) : _buffer(size) {}
  void recv(T event) { _buffer.push_back(event); };
  T getNext() { 
     return _buffer.front();
  }
  bool hasNext() { return !_buffer.empty();}
};
//______________________________________________________________________________
//
//______________________________________________________________________________
//
typedef enum { CONNECTED, DISCONNECTED , OPEN,CLOSE,RXD,TXD,IO_ERROR} Signal;