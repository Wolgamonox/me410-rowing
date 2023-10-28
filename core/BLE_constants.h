#ifndef BLE_CONSTANTS_H
#define BLE_CONSTANTS_H

#include "BLEDevice.h"

// The service of the leader we want to connect to / serve.
static BLEUUID serviceUUID("84527472-275e-4eea-8886-48b86d769be0");
// The characteristic  of the leader we want to listen to / notify.
static BLEUUID charUUID("2580e777-78f4-4ec6-9a33-574382957c28");

#endif  // BLE_CONSTANTS_H