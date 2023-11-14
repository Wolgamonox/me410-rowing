#ifndef LEADER_H
#define LEADER_H

class LeaderComService {
   public:
    // Initialize the communication service
    virtual bool init() = 0;

    // Checks for connection
    virtual bool isConnected() = 0;

    // Send a message to the follower
    virtual void send(const float& value) = 0;
};

#endif  // LEADER_H
