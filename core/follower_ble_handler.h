#ifndef FOLLOWER_BLE_HANDLER_H
#define FOLLOWER_BLE_HANDLER_H

#include <Arduino.h>
#include <BLEDevice.h>

#include "BLE_constants.h"

class FollowerBLEHandler {
   private:
    bool connected = false;
    bool doConnect = false;
    bool doScan = false;
    BLEAdvertisedDevice *leaderDevice;
    BLERemoteCharacteristic *pRemoteCharacteristic;

    bool
    connectToServer();

    static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic,
                               uint8_t *pData,
                               size_t length,
                               bool isNotify);

    class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
       private:
        BLEAdvertisedDevice **leaderDevice;
        bool *connected;
        bool *doConnect;
        bool *doScan;

       public:
        MyAdvertisedDeviceCallbacks(
            BLEAdvertisedDevice **leaderDevice,
            bool *connected,
            bool *doConnect,
            bool *doScan) : leaderDevice(leaderDevice),
                            connected(connected),
                            doConnect(doConnect),
                            doScan(doScan) {}

        // Called for each advertising BLE server.
        void onResult(BLEAdvertisedDevice advertisedDevice) {
            Serial.print("BLE Advertised Device found: ");
            Serial.println(advertisedDevice.toString().c_str());

            // We have found a device, let us now see if it contains the service we are looking for.
            if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
                Serial.printf("Found our device at address: %s\n", (*leaderDevice)->getAddress().toString().c_str());
                BLEDevice::getScan()->stop();
                *leaderDevice = new BLEAdvertisedDevice(advertisedDevice);

                *doConnect = true;
                *doScan = true;
            }
        }
    };

    class MyClientCallback : public BLEClientCallbacks {
       private:
        bool *connected;

       public:
        MyClientCallback(bool *connected) : connected(connected) {}

        void onConnect(BLEClient *pclient) {
        }

        void onDisconnect(BLEClient *pclient) {
            *connected = false;
        }
    };

   public:
    void setup();
    void loop();

    FollowerBLEHandler();
    ~FollowerBLEHandler();
};

#endif  // FOLLOWER_BLE_HANDLER_H