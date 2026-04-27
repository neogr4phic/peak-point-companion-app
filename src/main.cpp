#include <Arduino.h>
#include "display.h"
#include "encoder.h"
#include "scoring.h"
#include "rtc_time.h"
#include "ble_connection.h"

// Button state
static bool buttonPressed = false;
static unsigned long buttonPressStart = 0;
static bool longPressTriggered = false;

// Application states
enum AppState {
  STATE_NORMAL,
  STATE_FINISHING,
  STATE_FINISHED,
  STATE_BLE
};
static AppState appState = STATE_NORMAL;
static unsigned long finishingStartTime = 0;
static unsigned long finishedDisplayTime = 0;
static unsigned long lastDisplayUpdate = 0;

void setup() {
  encoderInit();
  displayInit();
  rtcTimeInit();
  bleInit();
  displayNormal(selectedLevel, dayScoreCounter);
}

void loop() {
  unsigned long now = millis();

  // Read encoder rotation (only in normal state)
  // Delta can be >1 when spinning fast, constrain clamps to valid range
  if (appState == STATE_NORMAL) {
    int8_t rotation = encoderReadRotation();
    if (rotation != 0) {
      int newLevel = (int)selectedLevel + (int)rotation;
      selectedLevel = (uint8_t)constrain(newLevel, 1, 9);
    }
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
        // Button held - enter finishing state after 500ms so countdown is the feedback
        if (!longPressTriggered && (now - buttonPressStart >= 500)) {
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
      // Defect fix: cancel if button released before countdown completes
      if (!currentButtonState) {
        appState = STATE_NORMAL;
        buttonPressed = false;
        longPressTriggered = false;
        lastDisplayUpdate = 0; // force immediate redraw
        break;
      }

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
      // Show "Day finished!" for 2 seconds then start BLE sync
      if (now - finishedDisplayTime >= 2000) {
        bleStart();
        appState = STATE_BLE;
      }
      break;
    }

    case STATE_BLE: {
      // BLE is active - no encoder input, just drive the BLE state machine
      if (bleUpdate()) {
        appState = STATE_NORMAL;
        buttonPressed = false;
        longPressTriggered = false;
        displayNormal(selectedLevel, dayScoreCounter);
      }
      break;
    }
  }
}