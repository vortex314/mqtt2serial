#include <MqttSerial.h>

MqttSerial::MqttSerial(Thread &thr)
    : Mqtt(thr),
      connected(false),
      keepAliveTimer(thr, TIMER_KEEP_ALIVE, 1000, true),
      connectTimer(thr, TIMER_CONNECT, 3000, true) {
  _rxdString.reserve(256);
  incoming.async(thr);
}
MqttSerial::~MqttSerial() {}

void MqttSerial::init() {
  INFO("MqttSerial started. ");
  txd.clear();
  rxd.clear();
  _hostPrefix = "src/";
  _hostPrefix += Sys::hostname();
  _hostPrefix += "/";
  _loopbackTopic = "dst/";
  _loopbackTopic += Sys::hostname();
  _loopbackTopic += "/system/loopback";
  _loopbackReceived = 0;

  outgoing.async(thread(), [&](const MqttMessage &m) {
    if (connected()) {
      String topic = _hostPrefix;
      topic += m.topic;
      publish(topic, m.message);
    }
  });

  // Sink<TimerMsg, 3> &me = *this;
  keepAliveTimer >> this;
  connectTimer >> this;

  Serial.setTimeout(0);
}

void MqttSerial::on(const TimerMsg &tm) {
  if (tm.id == TIMER_KEEP_ALIVE) {
    publish(_loopbackTopic, String("true"));
    outgoing.on({"system/alive", "true"});
  } else if (tm.id == TIMER_CONNECT) {
    if (Sys::millis() > (_loopbackReceived + 2000)) {
      connected = false;
      String topic = "dst/";
      topic += Sys::hostname();
      topic += "/#";
      subscribe(topic);
      publish(_loopbackTopic, "true");
    } else {
      connected = true;
    }
  } else {
    WARN("Invalid Timer Id");
  }
}

void MqttSerial::request() {}

void MqttSerial::onRxd(void *me) {
  MqttSerial *mqttSerial = (MqttSerial *)me;
  String s;
  s = Serial.readString();
//    INFO(" RXD >> %d", s.length());
  for (uint32_t i = 0; i < s.length(); i++)
    mqttSerial->handleSerialByte(s.charAt(i));
}

void MqttSerial::handleSerialByte(uint8_t b) {
  if (b == '\r' || b == '\n') {
    if (_rxdString.length() > 0) {
      rxdSerial(_rxdString);
    }
    _rxdString = "";
  } else {
    _rxdString += (char)b;
  }
}

void MqttSerial::rxdSerial(String &rxdString) {
  deserializeJson(rxd, rxdString);
  JsonArray array = rxd.as<JsonArray>();
  if (!array.isNull()) {
    if (array[1].as<String>() == _loopbackTopic) {
      _loopbackReceived = Sys::millis();
      connected = true;
    } else {
      String topic = array[1];
      INFO(" RXD >>> %s:%s",topic.substring(_hostPrefix.length()).c_str(),_hostPrefix.c_str());
      incoming.on({topic.substring(_hostPrefix.length()), array[2]});
    }
  } else {
    WARN(" parsing JSON array failed ");
  }
}

void MqttSerial::publish(String &topic, String message) {
  txd.clear();
  txd.add((int)CMD_PUBLISH);
  txd.add(topic);
  txd.add(message);
  txdSerial(txd);
}

void MqttSerial::subscribe(String &topic) {
  txd.clear();
  txd.add((int)CMD_SUBSCRIBE);
  txd.add(topic);
  txdSerial(txd);
}

void MqttSerial::txdSerial(JsonDocument &txd) {
  String output = "";
  serializeJson(txd, output);
  Serial.println(output.c_str());
  Serial.flush();
}
