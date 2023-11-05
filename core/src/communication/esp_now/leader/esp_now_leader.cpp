#include "esp_now_leader.h"

#include <WiFi.h>

#include "../mac_addresses.h"

EspNowLeader::EspNowLeader(float& valueRef) : valueRef(valueRef) {}

void onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    Serial.print("Packet to: ");
    // Copies the sender mac address to a string
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(macStr);
    Serial.print(" send status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

bool EspNowLeader::init() {
    WiFi.mode(WIFI_STA);

    if (esp_now_init() != ESP_OK) {
        return false;
    }

    // callback when data is sent
    if (esp_now_register_send_cb(onDataSent) != ESP_OK) {
        return false;
    }
    return true;
}

bool EspNowLeader::attach() {
    uint8_t macAddr[18];
    WiFi.macAddress(macAddr);

    if (macAddr == macAddress1) {
        // This is the first device
        // set the other mac address as the peer
        memcpy(peerInfo.peer_addr, macAddress2, 6);
    } else {
        // This is the second device
        // set the other mac address as the peer
        memcpy(peerInfo.peer_addr, macAddress1, 6);
    }

    // register peer
    peerInfo.channel = CHANNEL;
    peerInfo.encrypt = ENCRYPT;
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return false;
    }
    return true;
}

void EspNowLeader::notify() {
    esp_err_t result = esp_now_send(0, (uint8_t*)&valueRef, sizeof(valueRef));

    if (result == ESP_OK) {
        Serial.println("Sent with success");
    } else {
        Serial.println("Error sending the data");
    }
}
