#include <Arduino.h>
#include "config.h"
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
  STATE_BLE_PENDING,
  STATE_BLE
};
static AppState appState = STATE_NORMAL;
static unsigned long finishingStartTime = 0;
static unsigned long finishedDisplayTime = 0;
static unsigned long lastDisplayUpdate = 0;
static uint16_t historyScrollOffset = 0;

// ─── Button helper ────────────────────────────────────────────────────────────
enum ButtonEvent { BTN_NONE, BTN_SHORT_PRESS, BTN_LONG_PRESS };

static ButtonEvent processButton(bool current, unsigned long now) {
  if (current && !buttonPressed) {
    buttonPressed    = true;
    buttonPressStart = now;
    longPressTriggered = false;
    return BTN_NONE;
  }
  if (current && buttonPressed && !longPressTriggered) {
    if (now - buttonPressStart >= BTN_LONG_PRESS_MS) {
      longPressTriggered = true;
      return BTN_LONG_PRESS;
    }
    return BTN_NONE;
  }
  if (!current && buttonPressed) {
    bool wasShort = !longPressTriggered && (now - buttonPressStart >= BTN_DEBOUNCE_MS);
    buttonPressed      = false;
    longPressTriggered = false;
    return wasShort ? BTN_SHORT_PRESS : BTN_NONE;
  }
  return BTN_NONE;
}

void setup() {
  encoderInit();
  displayInit();
  rtcTimeInit();
  bleInit();
  displayNormal(selectedLevel, dayScoreCounter);
}

void loop() {
  unsigned long now = millis();

  // Always tick encoder for accurate position tracking
  int8_t rotation = encoderReadRotation();

  // Apply rotation to selectedLevel only in STATE_NORMAL
  if (appState == STATE_NORMAL && rotation != 0) {
    int newLevel = (int)selectedLevel + (int)rotation;
    selectedLevel = (uint8_t)constrain(newLevel, LEVEL_MIN, LEVEL_MAX);
  }
  bool currentButtonState = encoderButtonPressed();

  switch (appState) {
    case STATE_NORMAL: {
      ButtonEvent btn = processButton(currentButtonState, now);
      if (btn == BTN_SHORT_PRESS) { submitLevel(); }
      if (btn == BTN_LONG_PRESS)  { appState = STATE_FINISHING; finishingStartTime = now; }

      if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        displayNormal(selectedLevel, dayScoreCounter);
        lastDisplayUpdate = now;
      }
      break;
    }

    case STATE_FINISHING: {
      // Cancel if button released before countdown completes
      if (!currentButtonState) {
        appState = STATE_NORMAL;
        buttonPressed = false;
        longPressTriggered = false;
        lastDisplayUpdate = 0;
        break;
      }

      unsigned long elapsed = now - finishingStartTime;
      int secondsLeft = FINISHING_COUNTDOWN_S - (int)(elapsed / 1000);

      if (secondsLeft <= 0) {
        appState = STATE_FINISHED;
        historyScrollOffset = 0;
        finishedDisplayTime = now;
        displayFinished();
      } else {
        displayCountdown("finishing day in", secondsLeft);
      }
      break;
    }

    case STATE_FINISHED: {
      bool showingFinishedMsg = (now - finishedDisplayTime < FINISHED_MSG_DURATION_MS);

      if (!showingFinishedMsg) {
        // Scroll history list with encoder
        if (rotation != 0) {
          int newOffset = (int)historyScrollOffset + (int)rotation;
          int maxOffset = (int)dayScoreHistoryCount - HISTORY_VISIBLE_ROWS;
          if (maxOffset < 0) maxOffset = 0;
          historyScrollOffset = (uint16_t)constrain(newOffset, 0, maxOffset);
        }

        if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
          displayHistoryList(historyScrollOffset);
          lastDisplayUpdate = now;
        }

        // Long press in FINISHED to trigger BLE
        ButtonEvent btn = processButton(currentButtonState, now);
        if (btn == BTN_LONG_PRESS) { appState = STATE_BLE_PENDING; finishingStartTime = now; }
      }
      break;
    }

    case STATE_BLE_PENDING: {
      // Cancel if button released before countdown completes
      if (!currentButtonState) {
        appState = STATE_FINISHED;
        buttonPressed = false;
        longPressTriggered = false;
        lastDisplayUpdate = 0;
        break;
      }

      unsigned long elapsed = now - finishingStartTime;
      int secondsLeft = BLE_COUNTDOWN_S - (int)(elapsed / 1000);

      if (secondsLeft <= 0) {
        appState = STATE_BLE;
      } else {
        displayCountdown("BLE transmission in", secondsLeft);
      }
      break;
    }

    case STATE_BLE: {
      // BLE active - drive BLE state machine; reset day when done
      if (bleUpdate()) {
        resetDay();
        appState = STATE_NORMAL;
        buttonPressed = false;
        longPressTriggered = false;
        displayNormal(selectedLevel, dayScoreCounter);
      }
      break;
    }
  }
}