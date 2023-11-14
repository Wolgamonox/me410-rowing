#ifndef ESP_NOW_FOLLOWER_H
#define ESP_NOW_FOLLOWER_H

#include <esp_now.h>

#include "../../base/follower.h"
#include "../leader/esp_now_leader.h"

class EspNowFollower : public FollowerComService {
 public:
  bool init() override;

  bool isConnected() override;

  float getValue() override;

 private:
  static float value;

  static bool connected;

  static void onDataRecv(const uint8_t *mac, const uint8_t *incomingData,
                         int len);
};

#endif  // ESP_NOW_FOLLOWER_H