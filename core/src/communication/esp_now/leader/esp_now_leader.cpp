#include "esp_now_leader.h"

#include <WiFi.h>

#include "../../../definitions.h"
#include "../mac_addresses.h"

// define default values
esp_now_send_status_t EspNowLeader::lastPacketSendStatus = ESP_NOW_SEND_FAIL;

bool EspNowLeader::init() {
  WiFi.mode(WIFI_STA);
  if (esp_now_init() != ESP_OK) {
    debugPrintln("Error initializing ESP-NOW");
    return false;
  }

  esp_now_register_send_cb(onDataSent);

  // Set the peer info
  peerInfo.channel = CHANNEL;
  peerInfo.encrypt = ENCRYPT;

  // register peers
  memcpy(peerInfo.peer_addr, getPeerMacAddress(), 6);
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    debugPrintln("Failed to add peer");
    return false;
  }

  return true;
}

bool EspNowLeader::isConnected() {
  esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)&LEADER_HELLO, 4);
  return lastPacketSendStatus == ESP_NOW_SEND_SUCCESS;
}

void EspNowLeader::send(const float& value) {
  esp_err_t result = esp_now_send(peerInfo.peer_addr, (uint8_t*)&value, sizeof(value));
}

void EspNowLeader::onDataSent(const uint8_t* mac_addr, esp_now_send_status_t status) {
  lastPacketSendStatus = status;

  char macStr[18];
  debugPrint("Packet from: ");
  // Copies the sender mac address to a string
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
  debugPrint(macStr);
  debugPrint(" send status:\t");
  debugPrintln(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

uint8_t* EspNowLeader::getPeerMacAddress() {
  // get the mac address of the peer which is the one that is not our macaddr
  uint8_t macAddr[18];
  WiFi.macAddress(macAddr);

  if (macAddr[0] == macAddress1[0] && macAddr[1] == macAddress1[1] && macAddr[2] == macAddress1[2] &&
      macAddr[3] == macAddress1[3] && macAddr[4] == macAddress1[4] && macAddr[5] == macAddress1[5]) {
    return macAddress2;
  } else {
    return macAddress1;
  }
}