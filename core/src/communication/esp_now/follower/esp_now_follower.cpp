#include "esp_now_follower.h"

#include <WiFi.h>

// define default values
float EspNowFollower::value = 0.0f;
bool EspNowFollower::connected = false;

bool EspNowFollower::init() {
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return false;
    }

    esp_now_register_recv_cb(onDataRecv);

    return true;
}

bool EspNowFollower::isConnected() {
    return connected;
}

void EspNowFollower::update() {
    return;
}

float EspNowFollower::getValue() {
    return value;
}

void EspNowFollower::onDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
    memcpy(&value, incomingData, sizeof(value));

    if (!connected) {
        if (
            incomingData[0] == LEADER_HELLO[0] &&
            incomingData[1] == LEADER_HELLO[1] &&
            incomingData[2] == LEADER_HELLO[2] &&
            incomingData[3] == LEADER_HELLO[3]) {
            Serial.println("Received leader hello");
            connected = true;
        }
    }

    Serial.print("Bytes received: ");
    Serial.println(len);
    Serial.print("raw: ");
    Serial.printf("%02X:%02X:%02X:%02X\n", incomingData[0], incomingData[1], incomingData[2], incomingData[3]);
    Serial.printf("%02X:%02X:%02X:%02X\n", LEADER_HELLO[0], LEADER_HELLO[1], LEADER_HELLO[2], LEADER_HELLO[3]);
    Serial.print("value: ");
    Serial.println(value);
}
