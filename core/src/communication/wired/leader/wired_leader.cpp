#include <Wire.h>

#include "wired_leader.h"

bool WiredLeader::init() {
    Wire.begin();
}

bool WiredLeader::isConnected() {
}

void WiredLeader::send(const float& value) {
}