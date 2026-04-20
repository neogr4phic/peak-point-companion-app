#ifndef ENCODER_H
#define ENCODER_H

#include <Arduino.h>

#define PIN_CLK D0
#define PIN_DT  D1
#define PIN_SW  D2

void encoderInit();
int8_t encoderReadRotation();
bool encoderButtonPressed();

#endif
