#include <Arduino.h>
#include <MqttSerial.h>
#include <limero.h>

#include <deque>
#define PIN_LED 2
#define PIN_BUTTON 0
#include <LedBlinker.h>
#include <Button.h>
#include <Poller.h>

Thread mainThread("main");
LedBlinker ledBlinkerBlue(mainThread, PIN_LED, 100);
Button button1(mainThread, PIN_BUTTON);
Poller poller(mainThread);
MqttSerial mqtt(mainThread,Serial);
Log logger(1024);

LambdaSource<uint32_t> systemHeap([]() { return ESP.getFreeHeap(); });
LambdaSource<uint64_t> systemUptime([]() { return Sys::millis(); });
LambdaSource<const char *> systemHostname([]() { return Sys::hostname(); });
LambdaSource<const char *> systemBoard([]() { return Sys::board(); });
LambdaSource<const char *> systemCpu([]() { return Sys::cpu(); });
ValueSource<const char *> systemBuild = __DATE__ " " __TIME__;

void serialEvent() {
  INFO("-");
  MqttSerial::onRxd(&mqtt);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\r\n===== Starting  build " __DATE__ " " __TIME__);

  Sys::hostname(S(HOSTNAME));

  button1.init();
  ledBlinkerBlue.init();
  mqtt.init();

  mqtt.connected >> ledBlinkerBlue.blinkSlow;
  mqtt.connected >> poller.connected;
  mqtt.connected >> mqtt.toTopic<bool>("mqtt/connected");

  poller >> systemHeap >> mqtt.toTopic<uint32_t>("system/heap");
  poller >> systemUptime >> mqtt.toTopic<uint64_t>("system/upTime");
  poller >> systemBuild >> mqtt.toTopic<const char *>("system/build");
  poller >> systemHostname >> mqtt.toTopic<const char *>("system/hostname");
  poller >> systemBoard >> mqtt.toTopic<const char *>("system/board");
  poller >> systemCpu >> mqtt.toTopic<const char *>("system/cpu");
  poller >> button1 >> mqtt.toTopic<bool>("button/button1");

}

void loop() {
  mainThread.loop();
  if (Serial.available()) {
    MqttSerial::onRxd(&mqtt);
  }
}
