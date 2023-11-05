#ifndef FOLLOWER_H
#define FOLLOWER_H

class FollowerComService {
   public:
    // Receive callback
    virtual void receive() = 0;
};

#endif  // FOLLOWER_H
