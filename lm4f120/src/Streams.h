
#include <stdint.h>
#include <vector>
#include <functional>

//______________________________________________________________________________
//
class Event
{
public:
    union {
        struct
        {
            uint8_t source;
            uint8_t type;
        };
        uint16_t word;
    };
    Event();
    Event(uint16_t w);
    Event(int src, int t);
    bool operator==(Event &second);
};
//______________________________________________________________________________
//
template <class T>
class CircularBuffer
{
    T *_array;
    uint32_t _writePtr;
    uint32_t _readPtr;
    uint32_t _size;

public:
    CircularBuffer(uint32_t size);
    bool empty();
    bool pop(T &ret);
    void push(T value);
};
//______________________________________________________________________________
//
typedef std::function<void(Event &)> EventHandler;
//______________________________________________________________________________
//
class AbstractSink
{
public:
    virtual void recv(Event) = 0;
};
//______________________________________________________________________________
//
class AbstractFlow;

class AbstractSource
{
    std::vector<AbstractSink *> _sinks;
    static uint8_t _sourceIndex;
    uint8_t _id;

public:
    AbstractSource();
    void addSink(AbstractSink *_sink);
    virtual void operator>>(AbstractSink &sink);

    virtual void operator>>(EventHandler handler);
    virtual AbstractSource& operator>>(Event event);
    AbstractSource &operator>>(AbstractFlow &flow);

private:
    void send(Event event);
public:
    uint8_t id();
    void id(uint8_t id) ;
    AbstractSource&  map(int in,int out);
    AbstractSource& filter(int in);
    AbstractSource& on(int in);
};

//______________________________________________________________________________
//
class AbstractFlow : public AbstractSink, public AbstractSource
{
};


//______________________________________________________________________________
//
class Sink : public AbstractSink
{
    CircularBuffer<Event> _buffer;

public:
    Sink(uint32_t size) ;
    Sink(uint32_t size, EventHandler handler) ;
    void recv(Event event) ;
    bool getNext(Event &event) ;
};
//______________________________________________________________________________
//
class Filter : public AbstractFlow
{
    Event _match;

public:
    Filter(Event event);
    void recv(Event event);
};
class Map : public AbstractFlow
{
    int _in;
    int _out;

public:
    Map(Event in, Event out);
    Map(Event in[], Event out[]);
    void recv(Event event);
};
//______________________________________________________________________________
//
class Invoker : public AbstractSink
{
    Event &_event;
    EventHandler _handler;

public:
    void send(Event &event);
    Invoker(Event event, EventHandler handler);
};

class Source : public AbstractSource
{

public:
    Source(){};
    void on(Event event, EventHandler handler);
    AbstractFlow &filter(int type);
    void invoke(Event event, EventHandler handler) ;
};
