#ifndef ESP_NOW_LEADER_H
#define ESP_NOW_LEADER_H

#include <esp_now.h>

#include "../../base/leader.h"

const int CHANNEL = 43;
const bool ENCRYPT = false;

class EspNowLeader : public LeaderComService {
   public:
    bool init() override;

    bool isConnected() override;

    void send(const float& value) override;

   private:
    esp_now_peer_info_t peerInfo;

    static void onDataSent(const uint8_t* macAddr, esp_now_send_status_t status);

    uint8_t* getPeerMacAddress();
};

#endif  // ESP_NOW_LEADER_H