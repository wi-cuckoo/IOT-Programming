#include <SoftwareSerial.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  while(!Serial);
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c == 'a')
      digitalWrite(LED_BUILTIN, HIGH);   
  }
}
