#ifndef BLE_CONNECTION_H
#define BLE_CONNECTION_H

#include <Arduino.h>
#include "config.h"

void bleInit();
void bleStart();
bool bleUpdate();      // returns true when BLE session is complete
void bleStop();        // stops advertising and disconnects
bool bleIsConnected(); // true while a central is connected

#endif
