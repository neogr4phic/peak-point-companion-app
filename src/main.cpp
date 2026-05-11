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
  STATE_MENU,
  STATE_FINISHED,
  STATE_BLE
};
static AppState appState = STATE_NORMAL;
static unsigned long lastDisplayUpdate = 0;
static uint16_t historyScrollOffset = 0;

// Menu
enum MenuContext { MENU_CTX_NORMAL, MENU_CTX_FINISHED };
static MenuContext menuContext;
static uint8_t menuCursor = 0;

static const char* menuItemsNormal[]   = { "Finish day",   "[cancel]" };
static const char* menuItemsFinished[] = { "Sync via BLE", "[cancel]" };

// ─── Button helper ────────────────────────────────────────────────────────────
enum ButtonEvent { BTN_NONE, BTN_SHORT_PRESS, BTN_LONG_PRESS };

static ButtonEvent processButton(bool current, unsigned long now) {
  if (current && !buttonPressed) {
    buttonPressed      = true;
    buttonPressStart   = now;
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
  bool currentButtonState = encoderButtonPressed();

  switch (appState) {
    case STATE_NORMAL: {
      // Encoder adjusts selectedLevel
      if (rotation != 0) {
        int newLevel = (int)selectedLevel + (int)rotation;
        selectedLevel = (uint8_t)constrain(newLevel, LEVEL_MIN, LEVEL_MAX);
      }

      ButtonEvent btn = processButton(currentButtonState, now);
      if (btn == BTN_SHORT_PRESS) { submitLevel(); }
      if (btn == BTN_LONG_PRESS) {
        menuContext = MENU_CTX_NORMAL;
        menuCursor  = 0;
        appState    = STATE_MENU;
      }

      if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        displayNormal(selectedLevel, dayScoreCounter);
        lastDisplayUpdate = now;
      }
      break;
    }

    case STATE_MENU: {
      const char* const* items = (menuContext == MENU_CTX_NORMAL)
                                 ? menuItemsNormal
                                 : menuItemsFinished;

      // Encoder moves cursor between the two items
      if (rotation != 0) {
        int newCursor = (int)menuCursor + (int)rotation;
        menuCursor = (uint8_t)constrain(newCursor, 0, 1);
      }

      ButtonEvent btn = processButton(currentButtonState, now);
      if (btn == BTN_SHORT_PRESS) {
        if (menuCursor == 0) {
          // Action item selected
          if (menuContext == MENU_CTX_NORMAL) {
            historyScrollOffset = 0;
            appState = STATE_FINISHED;
          } else {
            bleStart();
            appState = STATE_BLE;
          }
        } else {
          // [cancel] selected — return to previous state
          appState = (menuContext == MENU_CTX_NORMAL) ? STATE_NORMAL : STATE_FINISHED;
        }
        lastDisplayUpdate = 0;
      }

      if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        displayMenu(items, 2, menuCursor);
        lastDisplayUpdate = now;
      }
      break;
    }

    case STATE_FINISHED: {
      // Encoder scrolls history list
      if (rotation != 0) {
        int newOffset = (int)historyScrollOffset + (int)rotation;
        int maxOffset = (int)dayScoreHistoryCount - HISTORY_VISIBLE_ROWS;
        if (maxOffset < 0) maxOffset = 0;
        historyScrollOffset = (uint16_t)constrain(newOffset, 0, maxOffset);
      }

      ButtonEvent btn = processButton(currentButtonState, now);
      if (btn == BTN_LONG_PRESS) {
        menuContext = MENU_CTX_FINISHED;
        menuCursor  = 0;
        appState    = STATE_MENU;
      }

      if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        displayHistoryList(historyScrollOffset);
        lastDisplayUpdate = now;
      }
      break;
    }

    case STATE_BLE: {
      // BLE active — drive BLE state machine; reset day when done
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