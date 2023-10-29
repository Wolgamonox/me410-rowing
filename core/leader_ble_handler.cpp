#include "leader_ble_handler.h"

bool LeaderBLEHandler::deviceConnected = false;
bool LeaderBLEHandler::oldDeviceConnected = false;

LeaderBLEHandler::LeaderBLEHandler() {
}

LeaderBLEHandler::~LeaderBLEHandler() {
}

void LeaderBLEHandler::setup() {
    // Create the BLE Device
    // Name has to be less than 5 chars !!!
    BLEDevice::init("RSD");

    // Create the BLE Server
    pServer = BLEDevice::createServer();
    pServer->setCallbacks(new MyServerCallbacks());

    // Create the BLE Service
    BLEService *pService = pServer->createService(serviceUUID);

    // Create a BLE Characteristic
    pCharacteristic = pService->createCharacteristic(
        charUUID,
        BLECharacteristic::PROPERTY_READ |
            BLECharacteristic::PROPERTY_NOTIFY |
            BLECharacteristic::PROPERTY_INDICATE);

    // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
    // Create a BLE Descriptor
    pCharacteristic->addDescriptor(new BLE2902());

    // Start the service
    pService->start();

    // Start advertising
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(serviceUUID);
    pAdvertising->setScanResponse(false);
    BLEDevice::startAdvertising();
    Serial.println("Waiting a client connection to notify...");
}

void LeaderBLEHandler::setKneeFlexion(float kneeFlexion) {
    this->kneeFlexion = kneeFlexion;
}

void LeaderBLEHandler::loop() {
    // Client connecting to server
    if (deviceConnected && !oldDeviceConnected) {
        oldDeviceConnected = deviceConnected;
        Serial.println("Device connected");
    }

    // Disconnecting
    if (!deviceConnected && oldDeviceConnected) {
        delay(500);                   // give the bluetooth stack the chance to get things ready
        pServer->startAdvertising();  // restart advertising
        Serial.println("Disconnection detected, start advertising");
        oldDeviceConnected = deviceConnected;
    }

    // notify changed value
    if (deviceConnected) {
        if (sendingKneeFlexion) {
            // Printing for debug
            Serial.println(kneeFlexion);

            // printf the bytes in hex of the kneeFlexion float
            // for (int i = 0; i < 4; i++) {
            //     Serial.print(((uint8_t *)&kneeFlexion)[i], HEX);
            //     Serial.print(" ");
            // }
            Serial.println();
            // note: big-endian hex notation
            pCharacteristic->setValue((uint8_t *)&kneeFlexion, 4);
            pCharacteristic->notify();
            delay(3000);  // bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
        }
    }
}