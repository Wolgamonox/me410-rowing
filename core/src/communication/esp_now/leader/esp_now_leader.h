#ifndef ESP_NOW_LEADER_H
#define ESP_NOW_LEADER_H

#include <esp_now.h>

#include "../../base/leader.h"

const int CHANNEL = 43;
const bool ENCRYPT = false;

class EspNowLeader : public LeaderComService {
   public:
    EspNowLeader(float& valueRef);

    bool init();

    bool attach();
    void notify();

   private:
    float& valueRef;
    esp_now_peer_info_t peerInfo;
};

#endif  // ESP_NOW_LEADER_H