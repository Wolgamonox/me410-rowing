#ifndef FOLLOWER_H
#define FOLLOWER_H

class FollowerComService {
   public:
    // Initialize the communication service
    virtual bool init() = 0;

    // Checks for connection
    virtual bool isConnected() = 0;

    // Check for new data
    virtual void update() = 0;

    // Get the last received value
    virtual float getValue() = 0;
};

#endif  // FOLLOWER_H
