#include "display.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET -1
static Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void displayInit() {
  if (!oled.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }
  oled.clearDisplay();
  oled.setTextColor(SSD1306_WHITE);
}

void displayNormal(uint8_t level, uint16_t score) {
  oled.clearDisplay();

  // selectedLevel - textSize(3), float left, vertically centered
  oled.setTextSize(3);
  int16_t levelY = (SCREEN_HEIGHT - 24) / 2;
  oled.setCursor(0, levelY);
  oled.print(level);

  // dayScoreCounter - textSize(4), float right, vertically centered
  oled.setTextSize(4);
  char scoreStr[5];
  snprintf(scoreStr, sizeof(scoreStr), "%u", score);
  int16_t scoreWidth = strlen(scoreStr) * 24;
  int16_t scoreX = SCREEN_WIDTH - scoreWidth;
  int16_t scoreY = (SCREEN_HEIGHT - 32) / 2;
  oled.setCursor(scoreX, scoreY);
  oled.print(score);

  oled.display();
}

void displayFinishing(int secondsLeft) {
  oled.clearDisplay();
  oled.setTextSize(1);

  const char* msg = "finishing day in";
  int16_t x1, y1;
  uint16_t w, h;
  oled.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);
  oled.setCursor((SCREEN_WIDTH - w) / 2, 4);
  oled.print(msg);

  oled.setTextSize(2);
  char timerStr[2];
  snprintf(timerStr, sizeof(timerStr), "%d", secondsLeft);
  oled.getTextBounds(timerStr, 0, 0, &x1, &y1, &w, &h);
  oled.setCursor((SCREEN_WIDTH - w) / 2, 16);
  oled.print(secondsLeft);

  oled.display();
}

void displayFinished() {
  oled.clearDisplay();
  oled.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  const char* line1 = "Day finished!";
  const char* line2 = "Good job!";

  oled.getTextBounds(line1, 0, 0, &x1, &y1, &w, &h);
  oled.setCursor((SCREEN_WIDTH - w) / 2, 4);
  oled.print(line1);

  oled.getTextBounds(line2, 0, 0, &x1, &y1, &w, &h);
  oled.setCursor((SCREEN_WIDTH - w) / 2, 18);
  oled.print(line2);

  oled.display();
}

// Renders msg centered on one line. If it is too wide for the screen,
// finds the last space that allows the first part to fit and splits there.
static void displayAutoText(const char* msg) {
  oled.clearDisplay();
  oled.setTextSize(1);

  int16_t x1, y1;
  uint16_t w, h;
  oled.getTextBounds(msg, 0, 0, &x1, &y1, &w, &h);

  if (w <= SCREEN_WIDTH) {
    // Fits on a single line — vertically center it
    oled.setCursor((SCREEN_WIDTH - w) / 2, (SCREEN_HEIGHT - h) / 2);
    oled.print(msg);
  } else {
    // Find the last space where line1 still fits within SCREEN_WIDTH
    int len = (int)strlen(msg);
    int splitPos = -1;
    for (int i = len - 1; i > 0; i--) {
      if (msg[i] == ' ') {
        char line1[32];
        int copyLen = (i < 31) ? i : 31;
        strncpy(line1, msg, copyLen);
        line1[copyLen] = '\0';
        uint16_t w1, h1;
        oled.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
        if (w1 <= SCREEN_WIDTH) {
          splitPos = i;
          break;
        }
      }
    }

    if (splitPos > 0) {
      char line1[32];
      char line2[32];
      int copyLen = (splitPos < 31) ? splitPos : 31;
      strncpy(line1, msg, copyLen);
      line1[copyLen] = '\0';
      strncpy(line2, msg + splitPos + 1, sizeof(line2) - 1);
      line2[sizeof(line2) - 1] = '\0';

      uint16_t w1, h1, w2, h2;
      oled.getTextBounds(line1, 0, 0, &x1, &y1, &w1, &h1);
      oled.getTextBounds(line2, 0, 0, &x1, &y1, &w2, &h2);
      oled.setCursor((SCREEN_WIDTH - w1) / 2, 6);
      oled.print(line1);
      oled.setCursor((SCREEN_WIDTH - w2) / 2, 18);
      oled.print(line2);
    } else {
      // No space found — start from left edge as fallback
      oled.setCursor(0, (SCREEN_HEIGHT - h) / 2);
      oled.print(msg);
    }
  }

  oled.display();
}

void displayBleConnecting() {
  displayAutoText("Connecting to Smartphone...");
}

void displayBleConnected() {
  displayAutoText("Smartphone connected!");
}

void displayBleSyncing() {
  displayAutoText("Syncing data...");
}

void displayBleSyncDone() {
  displayAutoText("Sync finished!");
}

void displayBleError() {
  displayAutoText("Bluetooth error!");
}
