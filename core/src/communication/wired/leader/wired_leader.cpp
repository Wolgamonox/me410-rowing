#include "wired_leader.h"

#include <Arduino.h>
#include <Wire.h>

bool WiredLeader::init() {
    return Wire.begin();
}




bool WiredLeader::isConnected() {
    // Read 4 bytes from the slave
    uint8_t bytesReceived = Wire.requestFrom(I2C_FOLLOWER_ADDRESS, 4);
    Serial.printf("requestFrom: %u\n", bytesReceived);
    if ((bool)bytesReceived) {  // If received more than zero bytes
        uint8_t temp[bytesReceived];
        Wire.readBytes(temp, bytesReceived);
        log_print_buf(temp, bytesReceived);
    }
}

void WiredLeader::send(const float& value) {
    // Wire.beginTransmission(I2C_FOLLOWER_ADDRESS);
    // Wire.println();
    // uint8_t error = Wire.endTransmission(true);
    // Serial.printf("Sending leader hello: %u\n", error);
}