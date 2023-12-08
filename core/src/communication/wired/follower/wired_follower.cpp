#include "wired_follower.h"

#include <Arduino.h>
#include <Wire.h>

#include "../../../definitions.h"

// define default values
float WiredFollower::value = 0.0f;
bool WiredFollower::connected = false;

bool WiredFollower::init() {
  Wire.onReceive(onReceive);
  Wire.onRequest(onRequest);
  return Wire.begin((uint8_t)I2C_FOLLOWER_ADDRESS, GPIO_NUM_22, GPIO_NUM_23, I2C_FREQ);
}

bool WiredFollower::isConnected() { return connected; }

float WiredFollower::getValue() { return value; }

void WiredFollower::onReceive(int bytesReceived) {
  uint8_t incomingData[bytesReceived];
  Wire.readBytes(incomingData, bytesReceived);

  memcpy(&value, incomingData, sizeof(value));
  debugPrintln(value, 4);
}

void WiredFollower::onRequest() {
  Wire.write(ACK_CHAR);
  connected = true;
}