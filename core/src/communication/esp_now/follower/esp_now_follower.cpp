#include "esp_now_follower.h"

#include <WiFi.h>

bool EspNowFollower::init() {
    WiFi.mode(WIFI_STA);

    // Init ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
    }


    esp_now_register_recv_cb(onDataRecv);
}

bool EspNowFollower::isConnected() {
}

bool EspNowFollower::update() {
}

float EspNowFollower::getValue() {
}

void EspNowFollower::onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len) {
    
}
