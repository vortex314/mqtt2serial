#include <Arduino.h>
#define ARDUINOJSON_ENABLE_PROGMEM 0
#include <ArduinoJson.h>
#undef min
#undef max
#include <vector>
#include <string>
#include <stdio.h>

#define LOG(s)     printf("%ld %s:%d : %s",millis(), __FILE__,__LINE__,s)

class Sys {
public:
  static std::string hostname;
  static std::string cpu;
  static std::string build;
};

//_______________________________________________________________________________________________________________
//
class Timer {
private:
  uint32_t _delta;
  uint64_t _timeout;
  bool _repeat;
  bool _actif;

public:
  Timer(uint32_t delta, bool repeat, bool actif);
  bool isRepeating();
  void repeat(bool rep);
  void start();
  void stop();
  bool timeout();
  void timeout(uint32_t delay);
};

//_______________________________________________________________________________________________________________
//
class ProtoThread {
  static std::vector<ProtoThread *> *_pts;
  typedef unsigned short LineNumber;
  uint64_t _timeout;
  Timer _defaultTimer;
  static const LineNumber LineNumberInvalid = (LineNumber)(-1);
  static std::vector<ProtoThread *> *pts();

public:
  LineNumber _ptLine;
  ProtoThread();
  virtual ~ProtoThread();
  bool timeout();
  void timeout(uint32_t delay);
  virtual void loop() = 0;
  virtual void setup() = 0;
  static void setupAll();
  static void loopAll();
  void restart();
  void stop();
  bool isRunning();
  bool isReady();
};

// Declare start of protothread (use at start of Run() implementation).
#define PT_BEGIN()                                                             \
  bool ptYielded = true;                                                       \
  switch (_ptLine) {                                                           \
  case 0:

// Stop protothread and end it (use at end of Run() implementation).
#define PT_END()                                                               \
  default:;                                                                    \
    }                                                                          \
    stop();                                                                    \
    return ;

// Cause protothread to wait until given condition is true.
#define PT_WAIT_UNTIL(condition)                                               \
  do {                                                                         \
    _ptLine = __LINE__;                                                        \
  case __LINE__:                                                               \
    if (!(condition))                                                          \
      return true;                                                             \
  } while (0)

// Cause protothread to wait while given condition is true.
#define PT_WAIT_WHILE(condition) PT_WAIT_UNTIL(!(condition))

// Cause protothread to wait until given child protothread completes.
#define PT_WAIT_THREAD(child) PT_WAIT_WHILE((child).dispatch(msg))

// Restart and spawn given child protothread and wait until it completes.
#define PT_SPAWN(child)                                                        \
  do {                                                                         \
    (child).restart();                                                         \
    PT_WAIT_THREAD(child);                                                     \
  } while (0)

// Restart protothread's execution at its PT_BEGIN.
#define PT_RESTART()                                                           \
  do {                                                                         \
    restart();                                                                 \
    return true;                                                               \
  } while (0)

// Stop and exit from protothread.
#define PT_EXIT()                                                              \
  do {                                                                         \
    stop();                                                                    \
    return false;                                                              \
  } while (0)

// Yield protothread till next call to its Run().
#define PT_YIELD()                                                             \
  do {                                                                         \
    ptYielded = false;                                                         \
    _ptLine = __LINE__;                                                        \
  case __LINE__:                                                               \
    if (!ptYielded)                                                            \
      return true;                                                             \
  } while (0)

// Yield protothread until given condition is true.
#define PT_YIELD_UNTIL(condition)                                              \
  do {                                                                         \
    ptYielded = false;                                                         \
    _ptLine = __LINE__;                                                        \
  case __LINE__:                                                               \
    if (!ptYielded || !(condition))                                            \
      return ;                                                             \
  } while (0)
//_______________________________________________________________________________________________________________
//
//_______________________________________________________________________________________________________________
//

class MqttSerial : public ProtoThread {
public:
  typedef void (*MqttCallback)(std::string topic, std::string message);

private:
  StaticJsonDocument<256> txd;
  StaticJsonDocument<256> rxd;
  std::string rxdString;
  bool _connected = false;
  std::string _loopbackTopic;
  Stream& _stream;
  Timer _loopbackTimer;
  Timer _connectTimer;
  uint64_t _loopbackReceived;

  typedef enum { CMD = 0, TOPIC, MESSAGE, QOS, RETAIN, CRC_IDX } Idx;
  enum { CMD_SUBSCRIBE = 0, CMD_PUBLISH };
  MqttCallback _callback;

public:
  MqttSerial(Stream& stream );
  void setup();
  void loop();
  void onMqttPublish(MqttCallback callback) ;
  void rxdSerial(std::string s);
  void publish(std::string topic, std::string message) ;
  void subscribe(std::string topic);
  void sendSerial();
  bool isConnected();
};

//_______________________________________________________________________________________________________________
//

int freeMemory();
