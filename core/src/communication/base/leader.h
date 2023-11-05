#ifndef LEADER_H
#define LEADER_H

#include "follower.h"

class LeaderComService {
   public:
    // Initialize the communication service
    virtual bool init() = 0;
    // Attach a follower to the leader
    virtual bool attach() = 0;
    // Notify the follower of a change
    virtual void notify() = 0;
};

#endif  // LEADER_H
