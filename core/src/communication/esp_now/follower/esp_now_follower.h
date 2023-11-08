#ifndef ESP_NOW_FOLLOWER_H
#define ESP_NOW_FOLLOWER_H

#include <esp_now.h>

#include "../../base/follower.h"

class EspNowFollower : public FollowerComService {
   public:
    bool init() override;

    bool isConnected() override;

    bool update() override;

    float getValue() override;

   private:
    float value = 0.0f;

    static void onDataRecv(const uint8_t * mac, const uint8_t *incomingData, int len);

};

#endif  // ESP_NOW_FOLLOWER_H