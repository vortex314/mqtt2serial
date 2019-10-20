// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

// Import libraries (BLEPeripheral depends on SPI)
#include <Arduino.h>
// Copyright (c) Sandeep Mistry. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.

/*
 * Serial Port over BLE
 * Create UART service compatible with Nordic's *nRF Toolbox* and Adafruit's *Bluefruit LE* iOS/Android apps.
 *
 * BLESerial class implements same protocols as Arduino's built-in Serial class and can be used as it's wireless
 * replacement. Data transfers are routed through a BLE service with TX and RX characteristics. To make the
 * service discoverable all UUIDs are NUS (Nordic UART Service) compatible.
 *
 * Please note that TX and RX characteristics use Notify and WriteWithoutResponse, so there's no guarantee
 * that the data will make it to the other end. However, under normal circumstances and reasonable signal
 * strengths everything works well.
 */


// Import libraries (BLEPeripheral depends on SPI)
#include <SPI.h>
#include <BLEPeripheral.h>
#include "BLESerial.h"

// define pins (varies per shield/board)
#define BLE_REQ   10
#define BLE_RDY   2
#define BLE_RST   9


void forward();
void loopback();
void spam();

// create ble serial instance, see pinouts above
BLESerial BLESerial(BLE_REQ, BLE_RDY, BLE_RST);


void setupBLE() {
  // custom services and characteristics can be added as well
  BLESerial.setLocalName("UART");

  Serial.begin(115200);
  BLESerial.begin();
}

void loopBLE() {
  BLESerial.poll();

  forward();
  loopback();
 // spam();
}


// forward received from Serial to BLESerial and vice versa
void forward() {
  if (BLESerial && Serial) {
    int byte;
    while ((byte = BLESerial.read()) > 0) Serial.write((char)byte);
    while ((byte = Serial.read()) > 0) BLESerial.write((char)byte);
  }
}

// echo all received data back
void loopback() {
  if (BLESerial) {
    int byte;
    while ((byte = BLESerial.read()) > 0) BLESerial.write(byte);
  }
}

// periodically sent time stamps
void spam() {
  if (BLESerial) {
    BLESerial.print(millis());
    BLESerial.println(" tick-tacks!");
    delay(1000);
  }
}

#include <Arduino.h>

#include "proto.h"

//_______________________________________________________________________________________________________________
//

//_______________________________________________________________________________________________________________
//
#define PIN_LED 20
class LedBlinker : public ProtoThread {
  uint32_t _pin, _delay;

public:
  LedBlinker(uint32_t pin, uint32_t delay) {
    _pin = pin;
    _delay = delay;
  }
  void setup() {
    LOG("LedBlinker started.");
    pinMode(_pin, OUTPUT);
    digitalWrite(_pin, 1);
  }
  bool loop() {
    PT_BEGIN();
    while (true) {
      timeout(_delay);
      digitalWrite(_pin, 0);
      PT_YIELD_UNTIL(timeout());
      timeout(_delay);
      digitalWrite(_pin, 1);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
  void delay(uint32_t d) { _delay = d; }
};
//_______________________________________________________________________________________________________________
//
// LedBlinker ledBlinkerRed(LED_RED_PIN,100);
// LedBlinker ledBlinkerGreen(LED_GREEN_PIN,300);
//_______________________________________________________________________________________________________________
//
class Publisher : public ProtoThread {
  String _systemPrefix;
  MqttSerial &_mqtt;
  LedBlinker &_ledBlinker;

public:
  Publisher(MqttSerial &mqtt, LedBlinker &ledBlinker)
      : _mqtt(mqtt), _ledBlinker(ledBlinker){};
  void setup() {
    LOG("Publisher started");
    _systemPrefix = "src/" + Sys::hostname + "/system/";
  }
  bool loop() {
    PT_BEGIN();
    while (true) {
      if (_mqtt.isConnected()) {
        _mqtt.publish(_systemPrefix + "upTime", String(millis()));
        _mqtt.publish(_systemPrefix + "build", Sys::build);
        _mqtt.publish(_systemPrefix + "cpu", Sys::cpu);
        _mqtt.publish(_systemPrefix + "heap", String(freeMemory()));
        _ledBlinker.delay(1000);
      } else
        _ledBlinker.delay(100);
      timeout(1000);
      PT_YIELD_UNTIL(timeout());
    }
    PT_END();
  }
};
//
//_____________________________________ protothreads running _____
//

MqttSerial mqtt(Serial);
MqttSerial mqtt2(BLESerial);
LedBlinker ledBlinkerBlue(PIN_LED, 100);
Publisher publisher(mqtt, ledBlinkerBlue);

void mqttCallback(String topic, String message) {
  Serial.println(" RXD " + topic + "=" + message);
}

void setup() {
  Serial.begin(115200);
  LOG("===== Starting ProtoThreads  build " __DATE__ " " __TIME__);
  Sys::hostname = "nordic";
  Sys::cpu = "nrf51822";
  mqtt.onMqttPublish(mqttCallback);
  ProtoThread::setupAll();
  setupBLE();
}

void loop() { loopBLE();ProtoThread::loopAll(); }

