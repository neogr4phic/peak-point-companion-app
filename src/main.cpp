#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <RTClib.h>

RTC_Millis rtc;

// Display configuration
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Rotary Encoder Pins
const int PIN_CLK = D0;
const int PIN_DT  = D1;
const int PIN_SW  = D2;

// Level to Points mapping (index 0 unused)
const uint16_t levelToPoints[] = {0, 1, 3, 6, 15, 25, 45, 80, 130, 200};

// DayScore-Counter
uint16_t dayScoreCounter = 0;

// Virtual Scroll-Wheel
uint16_t selectedLevel = 1;

// DayScore-History
#define MAX_HISTORY_ENTRIES 100
struct DayScoreEntry {
  char timestamp[9]; // "HH:MM:SS"
  uint16_t level;
};
DayScoreEntry dayScoreHistory[MAX_HISTORY_ENTRIES];
uint16_t dayScoreHistoryCount = 0;

// Days-History
#define MAX_DAYS 30
struct DayEntry {
  char date[11]; // "dd.mm.yyyy"
  uint16_t score;
  DayScoreEntry history[MAX_HISTORY_ENTRIES];
  uint16_t historyCount;
};
DayEntry daysHistory[MAX_DAYS];
uint16_t daysHistoryCount = 0;

// Encoder state
int lastStateCLK;

// Button state
bool buttonPressed = false;
unsigned long buttonPressStart = 0;
bool longPressTriggered = false;

// Application states
enum AppState {
  STATE_NORMAL,
  STATE_FINISHING,
  STATE_FINISHED
};
AppState appState = STATE_NORMAL;
unsigned long finishingStartTime = 0;
unsigned long finishedDisplayTime = 0;
unsigned long lastDisplayUpdate = 0;

void getTimestamp(char* buffer) {
  DateTime now = rtc.now();
  snprintf(buffer, 9, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
}

void getDateString(char* buffer) {
  DateTime now = rtc.now();
  snprintf(buffer, 11, "%02d.%02d.%04d", now.day(), now.month(), now.year());
}

void submitLevel() {
  if (dayScoreHistoryCount < MAX_HISTORY_ENTRIES) {
    getTimestamp(dayScoreHistory[dayScoreHistoryCount].timestamp);
    dayScoreHistory[dayScoreHistoryCount].level = selectedLevel;
    dayScoreHistoryCount++;
  }

  uint16_t points = levelToPoints[selectedLevel];
  uint32_t newScore = (uint32_t)dayScoreCounter + points;
  dayScoreCounter = (newScore > 9999) ? 9999 : (uint16_t)newScore;
}

void finishDay() {
  if (daysHistoryCount < MAX_DAYS) {
    getDateString(daysHistory[daysHistoryCount].date);
    daysHistory[daysHistoryCount].score = dayScoreCounter;
    daysHistory[daysHistoryCount].historyCount = dayScoreHistoryCount;
    for (uint16_t i = 0; i < dayScoreHistoryCount; i++) {
      daysHistory[daysHistoryCount].history[i] = dayScoreHistory[i];
    }
    daysHistoryCount++;
  }

  // Reset daily values (daysHistory is preserved)
  dayScoreCounter = 0;
  dayScoreHistoryCount = 0;
  selectedLevel = 1;
}

void displayNormal() {
  display.clearDisplay();

  // selectedLevel - textSize(3), float left, vertically centered
  display.setTextSize(3);
  int16_t levelY = (SCREEN_HEIGHT - 24) / 2;
  display.setCursor(0, levelY);
  display.print(selectedLevel);

  // dayScoreCounter - textSize(4), float right, vertically centered
  display.setTextSize(4);
  char scoreStr[5];
  snprintf(scoreStr, sizeof(scoreStr), "%u", dayScoreCounter);
  int16_t scoreWidth = strlen(scoreStr) * 24;
  int16_t scoreX = SCREEN_WIDTH - scoreWidth;
  int16_t scoreY = (SCREEN_HEIGHT - 32) / 2;
  display.setCursor(scoreX, scoreY);
  display.print(dayScoreCounter);

  display.display();
}

void displayFinishing(int secondsLeft) {
  display.clearDisplay();
  display.setTextSize(1);

  const char* msg = "finishing day in";
  int16_t x1, y1;
  uint16_t w, h;
  display.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 4);
  display.print(msg);

  display.setTextSize(2);
  char timerStr[2];
  snprintf(timerStr, sizeof(timerStr), "%d", secondsLeft);
  display.getTextBounds(timerStr, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 16);
  display.print(secondsLeft);

  display.display();
}

void displayFinished() {
  display.clearDisplay();
  display.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  const char* line1 = "Day finished!";
  const char* line2 = "Good job!";

  display.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 4);
  display.print(line1);

  display.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
  display.setCursor((SCREEN_WIDTH - w) / 2, 18);
  display.print(line2);

  display.display();
}

void setup() {
  pinMode(PIN_CLK, INPUT_PULLUP);
  pinMode(PIN_DT,  INPUT_PULLUP);
  pinMode(PIN_SW,  INPUT_PULLUP);

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }

  // Initialize software RTC with compile time
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));

  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);
  lastStateCLK = digitalRead(PIN_CLK);

  displayNormal();
}

void loop() {
  unsigned long now = millis();

  // Read encoder rotation (only in normal state)
  int currentStateCLK = digitalRead(PIN_CLK);
  if (currentStateCLK != lastStateCLK && currentStateCLK == 1) {
    if (appState == STATE_NORMAL) {
      if (digitalRead(PIN_DT) != currentStateCLK) {
        if (selectedLevel > 1) selectedLevel--;
      } else {
        if (selectedLevel < 9) selectedLevel++;
      }
    }
  }
  lastStateCLK = currentStateCLK;

  // Read button
  bool currentButtonState = (digitalRead(PIN_SW) == LOW);

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

      // Update display (throttled)
      if (now - lastDisplayUpdate >= 50) {
        displayNormal();
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
        displayNormal();
      }
      break;
    }
  }
}