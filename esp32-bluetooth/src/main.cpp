//This example code is in the Public Domain (or CC0 licensed, at your option.)
//By Evandro Copercini - 2018
//
//This example creates a bridge between Serial and Classical Bluetooth (SPP)
//and also demonstrate that SerialBT have the same functionalities of a normal Serial

#include "BluetoothSerial.h"
#include <Arduino.h>
#include <MqttSerial.h>
#include <limero.h>
#include <LedBlinker.h>
#include <Button.h>
#include <Poller.h>

#define PIN_LED 2
#define PIN_BUTTON 0

#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif

BluetoothSerial SerialBT;

void loopBT()
{
  if (Serial.available())
  {
    SerialBT.write(Serial.read());
  }
  if (SerialBT.available())
  {
    Serial.write(SerialBT.read());
  }
  delay(20);
}

Thread mainThread("main");
LedBlinker ledBlinkerBlue(mainThread, PIN_LED, 100);
Button button1(mainThread, PIN_BUTTON);
Poller poller(mainThread);
MqttSerial mqtt(mainThread, SerialBT);
Log logger(1024);

LambdaSource<uint32_t> systemHeap([]() { return ESP.getFreeHeap(); });
LambdaSource<uint64_t> systemUptime([]() { return Sys::millis(); });
LambdaSource<const char *> systemHostname([]() { return Sys::hostname(); });
LambdaSource<const char *> systemBoard([]() { return Sys::board(); });
LambdaSource<const char *> systemCpu([]() { return Sys::cpu(); });
ValueSource<const char *> systemBuild = __DATE__ " " __TIME__;

void serialBTEvent()
{
  INFO("-");
  MqttSerial::onRxd(&mqtt);
}

uint8_t macPc[] = {0x00, 0x15, 0x83, 0x47, 0x43, 0x5F};
uint8_t macMe[] = {0x30, 0xae, 0xa4, 0x1d, 0x81, 0x06}; // 30:AE:A4:1D:81:06

void setup()
{
  Serial.begin(115200);
  SerialBT.begin("ESP32-BT"); //Bluetooth device nam
  SerialBT.connect(macPc);
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

void loop()
{
  mainThread.loop();
  if (SerialBT.available())
  {
    MqttSerial::onRxd(&mqtt);
  }
}
