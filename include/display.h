#ifndef DISPLAY_H
#define DISPLAY_H

#include <Arduino.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32

void displayInit();
void displayNormal(uint8_t level, uint16_t score);
void displayFinishing(int secondsLeft);
void displayFinished();

#endif
