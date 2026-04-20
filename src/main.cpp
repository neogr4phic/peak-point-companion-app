#include <Arduino.h>
#include "display.h"
#include "encoder.h"
#include "scoring.h"
#include "rtc_time.h"

// Button state
static bool buttonPressed = false;
static unsigned long buttonPressStart = 0;
static bool longPressTriggered = false;

// Application states
enum AppState {
  STATE_NORMAL,
  STATE_FINISHING,
  STATE_FINISHED
};
static AppState appState = STATE_NORMAL;
static unsigned long finishingStartTime = 0;
static unsigned long finishedDisplayTime = 0;
static unsigned long lastDisplayUpdate = 0;

void setup() {
  encoderInit();
  displayInit();
  rtcTimeInit();
  displayNormal(selectedLevel, dayScoreCounter);
}

void loop() {
  unsigned long now = millis();

  // Read encoder rotation (only in normal state)
  if (appState == STATE_NORMAL) {
    int8_t rotation = encoderReadRotation();
    if (rotation > 0 && selectedLevel < 9) selectedLevel++;
    if (rotation < 0 && selectedLevel > 1) selectedLevel--;
  }

  // Read button
  bool currentButtonState = encoderButtonPressed();

  switch (appState) {
    case STATE_NORMAL: {
      if (currentButtonState && !buttonPressed) {
        // Button just pressed
        buttonPressed = true;
        buttonPressStart = now;
        longPressTriggered = false;
      } else if (currentButtonState && buttonPressed) {
        // Button held - check for long press (3 seconds)
        if (!longPressTriggered && (now - buttonPressStart >= 3000)) {
          longPressTriggered = true;
          appState = STATE_FINISHING;
          finishingStartTime = now;
        }
      } else if (!currentButtonState && buttonPressed) {
        // Button released
        if (!longPressTriggered && (now - buttonPressStart >= 50)) {
          // Short press - submit level
          submitLevel();
        }
        buttonPressed = false;
        longPressTriggered = false;
      }

      // Update display (throttled, 1000ms/50ms=20 FPS)
      if (now - lastDisplayUpdate >= 50) {
        displayNormal(selectedLevel, dayScoreCounter);
        lastDisplayUpdate = now;
      }
      break;
    }

    case STATE_FINISHING: {
      unsigned long elapsed = now - finishingStartTime;
      int secondsLeft = 3 - (int)(elapsed / 1000);

      if (secondsLeft <= 0) {
        finishDay();
        appState = STATE_FINISHED;
        finishedDisplayTime = now;
        displayFinished();
      } else {
        displayFinishing(secondsLeft);
      }
      break;
    }

    case STATE_FINISHED: {
      // Show message for 3 seconds then return to normal
      if (now - finishedDisplayTime >= 3000) {
        appState = STATE_NORMAL;
        buttonPressed = false;
        longPressTriggered = false;
        displayNormal(selectedLevel, dayScoreCounter);
      }
      break;
    }
  }
}