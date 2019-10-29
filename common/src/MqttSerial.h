//_______________________________________________________________________________________________________________
//
#include <Arduino.h>
#include <ProtoThread.h>
#include <Streams.h>

typedef struct MqttMessage {
  String topic;
  String message;
} MqttMessage;

class MqttSerial : public ProtoThread,
                   public BufferedSink<MqttMessage>,
                   public Source<MqttMessage> {
public:
private:
  StaticJsonDocument<256> txd;
  StaticJsonDocument<256> rxd;
  String rxdString;
  bool _connected = false;
  String _loopbackTopic;
  Stream &_stream;
  Timer _loopbackTimer;
  Timer _connectTimer;
  uint64_t _loopbackReceived;
  String _hostPrefix;

  enum { CMD_SUBSCRIBE = 0, CMD_PUBLISH };

public:
  Source<bool> connected;
  MqttSerial(Stream &stream);
  ~MqttSerial();
  void setup();
  void loop();
  void rxdSerial(String s);
  void publish(String topic, String message);
  void subscribe(String topic);
  void sendSerial();
  bool isConnected();
};
//_______________________________________________________________________________________________________________
//
template <class T> class ToMqtt : public Flow<T, MqttMessage> {
  String _name;

public:
  ToMqtt(String name) : _name(name){};
  void recv(T event) {
    String s = "";
    DynamicJsonDocument doc(100);
    JsonVariant variant = doc.to<JsonVariant>();
    variant.set(event);
    serializeJson(doc, s);
    this->emit({_name, s});
    // emit doesn't work as such
    // https://stackoverflow.com/questions/9941987/there-are-no-arguments-that-depend-on-a-template-parameter
  }
};
//_______________________________________________________________________________________________________________
//
template <class T> class FromMqtt : public Flow<MqttMessage, T> {
  String _name;

public:
  FromMqtt(String name) : _name(name){};
  void recv(MqttMessage mqttMessage) {
    if (mqttMessage.topic != _name) return;
    DynamicJsonDocument doc(100);
    auto error = deserializeJson(doc, mqttMessage.message);
    if (error) {
      LOG(" failed parsing " + mqttMessage.message + " " + error.c_str());
      return;
    }
    JsonVariant variant = doc.as<JsonVariant>();
    if (variant.isNull()) {
      LOG(" failed variant parsing " + mqttMessage.message);
      return;
    }
    T value = variant.as<T>();
    this->emit(value);
    // emit doesn't work as such
    // https://stackoverflow.com/questions/9941987/there-are-no-arguments-that-depend-on-a-template-parameter
  }
};

int freeMemory();
