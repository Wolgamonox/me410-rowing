#ifndef FOLLOWER_BLE_HANDLER_H
#define FOLLOWER_BLE_HANDLER_H

#include <Arduino.h>
#include <BLEDevice.h>

#include "BLE_constants.h"

class FollowerBLEHandler {
   private:
    static bool doConnect;
    static bool connected;
    static bool doScan;
    static BLERemoteCharacteristic* pRemoteCharacteristic;
    static BLEAdvertisedDevice* leaderDevice;

    bool connectToServer();

    class MyClientCallback : public BLEClientCallbacks {
        void onConnect(BLEClient* pclient) {
        }

        void onDisconnect(BLEClient* pclient) {
            connected = false;
        }
    };

    /**
     * Scan for BLE servers and find the first one that advertises the service we are looking for.
     */
    class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
        /**
         * Called for each advertising BLE server.
         */
        void onResult(BLEAdvertisedDevice advertisedDevice) {
            Serial.print("BLE Advertised Device found: ");
            Serial.println(advertisedDevice.toString().c_str());

            // We have found a device, let us now see if it contains the service we are looking for.
            if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
                BLEDevice::getScan()->stop();
                leaderDevice = new BLEAdvertisedDevice(advertisedDevice);
                doConnect = true;
                doScan = true;

            }  // Found our server
        }      // onResult
    };

    static void notifyCallback(
        BLERemoteCharacteristic* pBLERemoteCharacteristic,
        uint8_t* pData,
        size_t length,
        bool isNotify);

   public:
    void setup();
    void loop();

    FollowerBLEHandler();
    ~FollowerBLEHandler();
};

#endif  // FOLLOWER_BLE_HANDLER_H