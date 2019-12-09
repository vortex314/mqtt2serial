#include <Arduino.h>

void setup() {
  Serial.begin(115200);
}

void loop() {
Serial.println("[1,\"src/myTopic/time\","+String(millis())+"]");
delay(100);
Serial.println("[1,\"src/myTopic/build\",\"" __DATE__ " " __TIME__ "\"]");
delay(100);
}