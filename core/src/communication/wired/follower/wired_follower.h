#ifndef WIRED_FOLLOWER_H
#define WIRED_FOLLOWER_H

#include "../../base/follower.h"

const int I2C_FOLLOWER_ADDRESS = 0x43;
const int I2C_FREQ = 100000;
const char ACK_CHAR = 'A';

class WiredFollower : public FollowerComService {
 public:
  bool init() override;
  bool isConnected() override;
  float getValue() override;

 private:
   static void onReceive(int bytesReceived);
   static void onRequest();
   static float value;
   static bool connected;
};

#endif  // WIRED_FOLLOWER_H