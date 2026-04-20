#include "encoder.h"

static int lastStateCLK;

void encoderInit() {
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT,  INPUT_PULLUP);
  pinMode(PIN_SW,  INPUT_PULLUP);
  lastStateCLK = digitalRead(PIN_CLK);
}

int8_t encoderReadRotation() {
  int8_t direction = 0;
  int currentStateCLK = digitalRead(PIN_CLK);

  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    if (digitalRead(PIN_DT) != currentStateCLK) {
      direction = -1;
    } else {
      direction = 1;
    }
  }
  lastStateCLK = currentStateCLK;

  return direction;
}

bool encoderButtonPressed() {
  return (digitalRead(PIN_SW) == LOW);
}
