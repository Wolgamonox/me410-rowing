#include "wired_follower.h"

#include <Arduino.h>
#include <Wire.h>

// define default values
float WiredFollower::value = 0.0f;
bool WiredFollower::connected = false;

bool WiredFollower::init() {
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  return Wire.begin(I2C_FOLLOWER_ADDRESS);
}

bool WiredFollower::isConnected() { return connected; }

float WiredFollower::getValue() { return value; }

void WiredFollower::onReceive(int bytesReceived) {
  uint8_t incomingData[bytesReceived];
  Wire.readBytes(incomingData, bytesReceived);

  memcpy(&value, incomingData, sizeof(value));
  Serial.println(value, 4);
}

void WiredFollower::onRequest() {
  Wire.write(ACK_CHAR);
  connected = true;
}