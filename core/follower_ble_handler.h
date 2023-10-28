#ifndef FOLLOWER_BLE_HANDLER_H
#define FOLLOWER_BLE_HANDLER_H

#include <BLEDevice.h>
#include "BLE_constants.h"

class FollowerBLEHandler {
   private:
    

   public:
    void setup();
    void loop();

    FollowerBLEHandler();
    ~FollowerBLEHandler();
};

#endif  // FOLLOWER_BLE_HANDLER_H