#include "follower_ble_handler.h"

FollowerBLEHandler::FollowerBLEHandler() {
}

FollowerBLEHandler::~FollowerBLEHandler() {
}

void FollowerBLEHandler::setup() {
    // Retrieve a Scanner and set the callback we want to use to be informed when we
    // have detected a new device.  Specify that we want active scanning and start the
    // scan to run for 5 seconds.
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->setActiveScan(true);
    pBLEScan->start(5, false);
}

void FollowerBLEHandler::loop() {
    
}