#ifndef ESP_NOW_FOLLOWER_H
#define ESP_NOW_FOLLOWER_H

#include <esp_now.h>

#include "../../base/follower.h"
#include "../mac_addresses.h"

class EspNowFollower : public FollowerComService {
   public:
    EspNowLeader(float& valueRef);

    void receive();

   private:
    float& valueRef;
};

#endif  // ESP_NOW_FOLLOWER_H