#ifndef LEADER_BLE_HANDLER_H
#define LEADER_BLE_HANDLER_H

#include <Arduino.h>
#include <BLE2902.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>

#include "BLE_constants.h"

class LeaderBLEHandler {
   private:
    BLEServer* pServer = NULL;
    BLECharacteristic* pCharacteristic = NULL;
    static bool deviceConnected;
    static bool oldDeviceConnected;
    float kneeFlexion = 0.0f;

    bool sendingKneeFlexion = false;

    class MyServerCallbacks : public BLEServerCallbacks {
        void onConnect(BLEServer* pServer) {
            deviceConnected = true;
            BLEDevice::startAdvertising();
        };

        void onDisconnect(BLEServer* pServer) {
            deviceConnected = false;
        }
    };

   public:
    void setup();
    void setKneeFlexion(float kneeFlexion);
    bool getSendingKneeFlexion() { return sendingKneeFlexion; }
    void setSendingKneeFlexion(bool sendingKneeFlexion) { this->sendingKneeFlexion = sendingKneeFlexion; }
    void loop();

    bool isConnected() { return deviceConnected; }

    LeaderBLEHandler();
    ~LeaderBLEHandler();
};

#endif  // LEADER_BLE_HANDLER_H