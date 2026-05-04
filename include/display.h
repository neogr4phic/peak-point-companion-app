#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

void displayInit();
void displayNormal(uint8_t level, uint16_t score);
void displayCountdown(const char* label, int secondsLeft);
void displayFinished();
void displayHistoryList(uint16_t scrollOffset);
void displayBleConnecting();
void displayBleConnected();
void displayBleSyncing();
void displayBleSyncDone();
void displayBleError();

#endif
