#include "encoder.h"
#include <RotaryEncoder.h>

// mathertel/RotaryEncoder: TWO03 latch mode works reliably with KY-040
// Built-in debouncing and simultaneous CLK+DT sampling eliminate glitches
static RotaryEncoder encoder(PIN_CLK, PIN_DT, RotaryEncoder::LatchMode::TWO03);
static long lastPosition = 0;

void encoderInit() {
  // CLK and DT pins are configured INPUT_PULLUP by the RotaryEncoder constructor
  pinMode(PIN_SW, INPUT_PULLUP);
  lastPosition = encoder.getPosition();
}

// Returns signed delta since last call. Can be >1 if encoder moved fast (natural acceleration).
int8_t encoderReadRotation() {
  encoder.tick();
  long pos = encoder.getPosition();
  int8_t delta = (int8_t)constrain(pos - lastPosition, -127, 127);
  lastPosition = pos;
  return delta;
}

bool encoderButtonPressed() {
  return (digitalRead(PIN_SW) == LOW);
}
