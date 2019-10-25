
#include <stdint.h>
#include <vector>
#include <functional>

void std::__throw_bad_function_call()
{
    while (1)
        ;
}
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
    Event()
    {
        word = 0;
    }
    Event(uint16_t w)
    {
        word = w;
    }
    Event(int src, int t)
    {
        source = src;
        type = t;
    }
    bool operator==(Event &second)
    {
        return word == second.word;
    }
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
    CircularBuffer(uint32_t size)
    {
        _array = new T[size];
        _size = size;
        _writePtr = 0;
        _readPtr = 0;
    };
    bool empty() { return _readPtr == _writePtr; };
    bool pop(T &ret)
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
    void push(T value)
    {
        if (_writePtr != _readPtr)
        {
            _array[_writePtr] = value;
            if (++_writePtr == _size)
                _writePtr = 0;
        }
    }
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
    AbstractSource() { _id = _sourceIndex++; }

    void addSink(AbstractSink *_sink)
    {
        _sinks.push_back(_sink);
    }
    virtual void operator>>(AbstractSink &sink)
    {
        _sinks.push_back(&sink);
    }

    virtual void operator>>(EventHandler handler);

    AbstractSource &operator>>(AbstractFlow &flow);


    void send(Event event)
    {
        for (AbstractSink *_sink : _sinks)
        {
            _sink->recv(event);
        }
    }
    uint8_t id() { return _id; };
    void id(uint8_t id) { _id = id; };
    AbstractSource&  map(int in,int out);
    AbstractSource& filter(int in);
    AbstractSource& on(int in);
};

//______________________________________________________________________________
//
class AbstractFlow : public AbstractSink, public AbstractSource
{
};

AbstractSource &AbstractSource::operator>>(AbstractFlow &flow)
{
    _sinks.push_back(&flow);
    return flow;
}
//______________________________________________________________________________
//
class Sink : public AbstractSink
{
    CircularBuffer<Event> _buffer;

public:
    Sink(uint32_t size) : _buffer(size){};
    Sink(uint32_t size, EventHandler handler) : _buffer(size){};
    void recv(Event event) { _buffer.push(event); };
    bool getNext(Event &event) { return _buffer.pop(event); };
};
//______________________________________________________________________________
//
class Filter : public AbstractFlow
{
    Event _match;

public:
    Filter(Event event)
    {
        _match = event;
    };
    void recv(Event event)
    {
        if (event == _match)
            send(event);
    }
};
class Map : public AbstractFlow
{
    int _in;
    int _out;

public:
    Map(Event in, Event out);
    Map(Event in[], Event out[]);
    void recv(Event event)
    {
        if (event.type == _in)
            send(_out);
    }
};
//______________________________________________________________________________
//
class Invoker : public AbstractSink
{
    Event &_event;
    EventHandler _handler;

public:
    void send(Event &event)
    {
        if (_event.word == event.word)
            _handler(event);
    }
    Invoker(Event event, EventHandler handler) : _event(event), _handler(handler){};
};

class Source : public AbstractSource
{

public:
    Source(){};
    void on(Event event, EventHandler handler)
    {
    }
    AbstractFlow &filter(int type)
    {
        AbstractFlow *flow = new Filter(Event(this->id(), type));
        operator>>(*flow);
        return *flow;
    }
    void invoke(Event event, EventHandler handler) {}
};

uint8_t AbstractSource::_sourceIndex = 1;

class Mqtt : public AbstractFlow
{
public:
    enum
    {
        CONNECTED,
        DISCONNECTED
    };
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
    static AbstractFlow &type(int type)
    {
        return *new On(type);
    }
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
    void recv(Event event)
    {
        _handler(event);
    }
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
    source >> sink;
    Source serial;
    Mqtt mqtt;
    Led led;

    Sink blinkFast(1, [&led](Event event) { led.blinkFast(); });

    mqtt.map(Mqtt::CONNECTED, Led::BLINK_SLOW) >> led;
    mqtt.map(Mqtt::DISCONNECTED, Led::BLINK_FAST) >> led;
    serial >> mqtt;

    mqtt.on(Mqtt::DISCONNECTED) >> [](Event event){};

    mqtt >> On::type(Mqtt::DISCONNECTED) >> InvokeDirect::call([&led](Event ev) { led.blinkFast(); });
    mqtt >> On::type(Mqtt::CONNECTED) >> InvokeDirect::call([&led](Event ev) { led.blinkSlow(); });
}