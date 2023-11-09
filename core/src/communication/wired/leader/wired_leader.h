#ifndef WIRED_LEADER_H
#define WIRED_LEADER_H

#include "../../base/leader.h"
#include "../follower/wired_follower.h"

class WiredLeader : public LeaderComService {
   public:
    bool init() override;
    bool isConnected() override;
    void send(const float& value) override;

   private:

};

#endif  // WIRED_LEADER_H
