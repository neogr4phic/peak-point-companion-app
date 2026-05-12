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
  STATE_HISTORY,
  STATE_SCORE,
  STATE_FLASH,
  STATE_BLE
};
static AppState appState = STATE_NORMAL;
static unsigned long lastDisplayUpdate = 0;
static uint16_t historyScrollOffset = 0;

// Menu
enum MenuContext { MENU_CTX_NORMAL, MENU_CTX_ENTRY, MENU_CTX_SCORE };
static MenuContext menuContext;
static uint8_t menuCursor = 0;

static const char* menuItemsNormal2[] = { "Finish day",       "Back" };
static const char* menuItemsNormal4[] = { "Finish day",       "Delete last", "Edit scores", "Back" };
static const char* menuItemsEntry[]   = { "Delete selection", "Delete all",  "Scroll list", "Back" };
static const char* menuItemsScore[]   = { "Transmit score",   "Dismiss score" };

// Flash state
static const char*  flashMsg         = nullptr;
static AppState     flashReturnState = STATE_NORMAL;
static unsigned long flashUntil      = 0;

static void enterFlash(const char* msg, AppState returnTo, unsigned long now) {
  flashMsg         = msg;
  flashReturnState = returnTo;
  flashUntil       = now + FLASH_DURATION_MS;
  appState         = STATE_FLASH;
  lastDisplayUpdate = 0;
}

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
      bool normalHasHistory = (menuContext == MENU_CTX_NORMAL) && (dayScoreHistoryCount > 0);
      const char* const* items;
      uint8_t itemCount;
      if (menuContext == MENU_CTX_NORMAL) {
        items     = normalHasHistory ? menuItemsNormal4 : menuItemsNormal2;
        itemCount = normalHasHistory ? 4 : 2;
      } else if (menuContext == MENU_CTX_ENTRY) {
        items     = menuItemsEntry;
        itemCount = 4;
      } else { // MENU_CTX_SCORE
        items     = menuItemsScore;
        itemCount = 2;
      }

      // Encoder moves cursor
      if (rotation != 0) {
        int newCursor = (int)menuCursor + (int)rotation;
        menuCursor = (uint8_t)constrain(newCursor, 0, itemCount - 1);
      }

      ButtonEvent btn = processButton(currentButtonState, now);
      if (btn == BTN_SHORT_PRESS) {
        if (menuContext == MENU_CTX_NORMAL) {
          if (menuCursor == 0) {                              // "Finish day"
            displayFinalScoreReset();
            appState = STATE_SCORE;
          } else if (menuCursor == 1 && normalHasHistory) {  // "Delete last"
            deleteHistoryEntry(dayScoreHistoryCount - 1);
            appState = STATE_NORMAL;
          } else if (menuCursor == 2 && normalHasHistory) {  // "Edit scores"
            historyScrollOffset = 0;
            appState = STATE_HISTORY;
          } else {                                            // "Back"
            appState = STATE_NORMAL;
          }
        } else if (menuContext == MENU_CTX_ENTRY) {
          if (menuCursor == 0) {                              // "Delete selection"
            deleteHistoryEntry(historyScrollOffset);
            int maxOffset = (int)dayScoreHistoryCount - HISTORY_VISIBLE_ROWS;
            if (maxOffset < 0) maxOffset = 0;
            if ((int)historyScrollOffset > maxOffset)
              historyScrollOffset = (uint16_t)maxOffset;
            enterFlash("Score deleted", STATE_HISTORY, now);
          } else if (menuCursor == 1) {                      // "Delete all"
            clearHistory();
            menuContext = MENU_CTX_NORMAL;
            menuCursor  = 0;
            enterFlash("List deleted", STATE_MENU, now);
          } else if (menuCursor == 2) {                      // "Scroll list"
            appState = STATE_HISTORY;
          } else {                                            // "Back"
            menuContext = MENU_CTX_NORMAL;
            menuCursor  = 0;
            appState    = STATE_MENU;
          }
        } else { // MENU_CTX_SCORE
          if (menuCursor == 0) {                              // "Transmit score"
            bleStart();
            appState = STATE_BLE;
          } else {                                            // "Dismiss score"
            resetDay();
            appState = STATE_NORMAL;
          }
        }
        lastDisplayUpdate = 0;
      }

      if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        displayMenu(items, itemCount, menuCursor);
        lastDisplayUpdate = now;
      }
      break;
    }

    case STATE_HISTORY: {
      // Encoder scrolls history list
      if (rotation != 0) {
        int newOffset = (int)historyScrollOffset + (int)rotation;
        int maxOffset = (int)dayScoreHistoryCount - HISTORY_VISIBLE_ROWS;
        if (maxOffset < 0) maxOffset = 0;
        historyScrollOffset = (uint16_t)constrain(newOffset, 0, maxOffset);
      }

      ButtonEvent btn = processButton(currentButtonState, now);
      if (btn == BTN_SHORT_PRESS && dayScoreHistoryCount > 0) {
        menuContext = MENU_CTX_ENTRY;
        menuCursor  = 0;
        appState    = STATE_MENU;
      }
      // No long-press in STATE_HISTORY

      if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        displayHistoryList(historyScrollOffset);
        lastDisplayUpdate = now;
      }
      break;
    }

    case STATE_SCORE: {
      ButtonEvent btn = processButton(currentButtonState, now);
      if (btn == BTN_SHORT_PRESS) {
        menuContext = MENU_CTX_SCORE;
        menuCursor  = 0;
        appState    = STATE_MENU;
      }
      // No long-press in STATE_SCORE

      if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        displayFinalScore(dayScoreCounter);
        lastDisplayUpdate = now;
      }
      break;
    }

    case STATE_FLASH: {
      // No encoder or button input accepted during flash
      if (now >= flashUntil) {
        appState = flashReturnState;
        lastDisplayUpdate = 0;
      } else if (now - lastDisplayUpdate >= DISPLAY_UPDATE_INTERVAL_MS) {
        displayMessage(flashMsg);
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