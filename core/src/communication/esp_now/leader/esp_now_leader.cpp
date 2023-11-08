#include "esp_now_leader.h"

#include <WiFi.h>

#include "../mac_addresses.h"

bool EspNowLeader::init() {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return false;
    }

    esp_now_register_send_cb(onDataSent);

    // Set the peer info
    peerInfo.channel = CHANNEL;
    peerInfo.encrypt = ENCRYPT;

    // register peer
    memcpy(peerInfo.peer_addr, getPeerMacAddress(), 6);
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Failed to add peer");
        return;
    }
}

bool EspNowLeader::isConnected() {
    uint8_t leaderHello[] = {0xDE, 0xAD, 0xBE, 0xEF};
    esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)&leaderHello, 4);
    return (result == ESP_OK);
}

void EspNowLeader::send(const float& value) {
    esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)&value, sizeof(value));
}

void EspNowLeader::onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    Serial.print("Packet from: ");
    // Copies the sender mac address to a string
    snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
             mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    Serial.print(macStr);
    Serial.print(" send status:\t");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

uint8_t* EspNowLeader::getPeerMacAddress() {
    // get the mac address of the peer which is the one that is not our macaddr
    uint8_t macAddr[18];
    WiFi.macAddress(macAddr);

    if (macAddr == macAddress1) {
        return macAddress2;
    } else {
        return macAddress1;
    }
}