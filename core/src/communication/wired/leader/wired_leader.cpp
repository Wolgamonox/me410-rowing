#include "wired_leader.h"

#include <Arduino.h>
#include <Wire.h>

#include "../../../definitions.h"

bool WiredLeader::init() { return Wire.begin(); }

bool WiredLeader::isConnected() {
  uint8_t bytesReceived = Wire.requestFrom(I2C_FOLLOWER_ADDRESS, 1);
  if ((bool)bytesReceived) {  // If received more than zero bytes
    uint8_t temp[bytesReceived];
    Wire.readBytes(temp, bytesReceived);
    if (temp[0] == ACK_CHAR) {
      return true;
    }
  }

  return false;
}

void WiredLeader::send(const float& value) {
  Wire.beginTransmission(I2C_FOLLOWER_ADDRESS);
  Wire.write((uint8_t*)&value, sizeof(value));
  uint8_t error = Wire.endTransmission(true);
  debugPrintf("endTransmission: %u\n", error);
}