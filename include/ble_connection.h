#ifndef BLE_CONNECTION_H
#define BLE_CONNECTION_H

#include <Arduino.h>

// Advertising timeout in ms before giving up and powering down BLE
#define BLE_ADVERTISE_TIMEOUT_MS 30000

void bleInit();
void bleStart();
bool bleUpdate(); // returns true when BLE session is complete
void bleStop();

#endif
