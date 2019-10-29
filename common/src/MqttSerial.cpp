#include <MqttSerial.h>

uint32_t startTime;
void timerStart() { startTime = millis(); }
void timerStop(String s, uint32_t delta) {
  uint32_t measured = (millis() - startTime);
  if (measured > delta) {
    LOG(" slow  " + s + " " + String(measured));
  }
}

MqttSerial::MqttSerial(Stream &stream)
    : BufferedSink<MqttMessage>(5), _stream(stream),
      _loopbackTimer(1000, true, true), _connectTimer(3000, true, true) {}

MqttSerial::~MqttSerial() {}

void MqttSerial::setup() {
  LOG("MqttSerial started. " + String((uint32_t)this));
  txd.clear();
  rxd.clear();
  _hostPrefix = "dst/" + Sys::hostname+"/";
  _loopbackTopic =  _hostPrefix + "system/loopback";
  _loopbackTimer.start();
  _connectTimer.start();
  _loopbackReceived = 0;
  _stream.setTimeout(0);
}

void MqttSerial::loop() {
  PT_BEGIN();
  timeout(1000);
  while (true) {
    PT_YIELD_UNTIL(_stream.available() || timeout() ||
                   _loopbackTimer.timeout() || hasNext());
    if (_stream.available()) {
      String s = _stream.readString();
      rxdSerial(s);
    };
    if (_connectTimer.timeout()) {
      if (millis() > (_loopbackReceived + 2000)) {
        _connected = false;
        connected.emit(false);
        subscribe("dst/" + Sys::hostname + "/#");
        publish(_loopbackTopic, "true");
      } else {
        _connected = true;
        connected.emit(true);
      }
      _connectTimer.start();
    }
    if (_loopbackTimer.timeout()) {
      publish(_loopbackTopic, "true");
      _loopbackTimer.start();
    }
    if (hasNext()) {
      MqttMessage m;
      m = getNext();
      if (_connected)
        publish("src/" + Sys::hostname + "/" + m.topic, m.message);
    }
  }
  PT_END();
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
bool MqttSerial::isConnected() { return _connected; }
