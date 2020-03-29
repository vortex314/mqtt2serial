#include <Arduino.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
Serial.println("[1,\"src/myTopic/time\","+String(millis())+"]");
delay(10);
}