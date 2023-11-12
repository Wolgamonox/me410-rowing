#include "wired_follower.h"

#include <Arduino.h>
#include <Wire.h>

bool WiredFollower::init() {
    return Wire.begin(I2C_FOLLOWER_ADDRESS);
}

bool WiredFollower::isConnected() {
    
}

void WiredFollower::update() {

}

float WiredFollower::getValue() {

}