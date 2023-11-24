#include <Arduino.h>

#define ENCODER_PIN A0

void setup() {
  Serial.begin(115200);
  Serial.println("Monitor started.");
}

void loop() {
  int encoderValue = analogRead(ENCODER_PIN);

  Serial.println(encoderValue);
  delay(100);
}
