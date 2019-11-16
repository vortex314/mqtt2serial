#include <MqttSerial.h>


#define TIMER_KEEP_ALIVE 1
#define TIMER_CONNECT 2
#define TIMER_SERIAL 3

MqttSerial::MqttSerial(Stream &stream)
    : _stream(stream), connected(false), incoming(20), outgoing(20),
      keepAliveTimer(TIMER_KEEP_ALIVE, 1000, true),
      connectTimer(TIMER_CONNECT, 3000, true),
      serialTimer(TIMER_SERIAL, 10, true) {}
MqttSerial::~MqttSerial() {}

void MqttSerial::init() {
  LOG("MqttSerial started. " );
  txd.clear();
  rxd.clear();
  _hostPrefix = "dst/" + Sys::hostname + "/";
  _loopbackTopic = _hostPrefix + "system/loopback";
  _loopbackReceived = 0;
  _stream.setTimeout(0);
  outgoing >> *this;
  *this >> incoming;
  Sink<TimerMsg>& me=*this;
  keepAliveTimer >> me;
  connectTimer >> me;
  serialTimer >> me;
  connected.emitOnChange(true);
}

void MqttSerial::onNext(TimerMsg tm) {
 //LOG(" timer : %lu ",tm.id);
  if (tm.id == TIMER_KEEP_ALIVE) {
    publish(_loopbackTopic, "true");
    outgoing.onNext({"system/alive","true"});
  } else if (tm.id == TIMER_CONNECT) {
    if (millis() > (_loopbackReceived + 2000)) {
      connected = false;
      subscribe("dst/" + Sys::hostname + "/#");
      publish(_loopbackTopic, "true");
    } else {
      connected = true;
    }
  } else if (tm.id == TIMER_SERIAL) {
 //   LOG("TIMER_SERIAL");
    if (_stream.available()) {
      String s = _stream.readString();
      rxdSerial(s);
    };
  } else {
    LOG("Invalid Timer Id");
  }
}

void MqttSerial::onNext(MqttMessage m) {
  if (connected()) {
    publish("src/" + Sys::hostname + "/" + m.topic, m.message);
  };
}

void MqttSerial::request() {
  if (connected()) {
    incoming.request();
    outgoing.request();
  }
  keepAliveTimer.request();
  connectTimer.request();
  serialTimer.request();
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
          } else {
            String topic = array[1];
            emit({topic.substring(_hostPrefix.length()), array[2]});
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
