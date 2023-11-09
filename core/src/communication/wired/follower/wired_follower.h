#ifndef WIRED_FOLLOWER_H
#define WIRED_FOLLOWER_H

#include "../../base/follower.h"

const int FOLLOWER_ADDRESS = 0x43;

class WiredFollower : public FollowerComService {
   public:
    bool init() override;
    bool isConnected() override;
    void update() override;
    float getValue() override;

   private:
};

#endif  // WIRED_FOLLOWER_H