#include "display.h"
#include "scoring.h"
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define OLED_RESET -1
static Adafruit_SSD1306 oled(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

void displayInit() {
  if (!oled.begin(SSD1306_SWITCHCAPVCC, OLED_I2C_ADDR)) {
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

// Draws up/down scroll indicator triangles in the top/bottom-right corner.
static void displayScrollTriangles(bool canUp, bool canDown) {
  if (canUp)   oled.fillTriangle(123, 2, 119, 8,  127, 8,  SSD1306_WHITE);
  if (canDown) oled.fillTriangle(123, 29, 119, 23, 127, 23, SSD1306_WHITE);
}

void displayMenu(const char* const items[], uint8_t count, uint8_t cursor) {
  oled.clearDisplay();
  oled.setTextSize(1);

  // Top row: selected item
  oled.setCursor(0, MENU_ROW0_Y);
  oled.print("> ");
  oled.print(items[cursor]);

  // Bottom row: next item preview (if any)
  if (cursor + 1 < count) {
    oled.setCursor(0, MENU_ROW0_Y + MENU_ROW_SPACING);
    oled.print("  ");
    oled.print(items[cursor + 1]);
  }

  displayScrollTriangles(cursor > 0, cursor + 2 < count);

  oled.display();
}

// Renders msg centered on one line; auto-splits at a word boundary if too wide.
void displayMessage(const char* msg) {
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
  oled.clearDisplay();
  oled.setTextSize(1);

  int16_t x1, y1;
  uint16_t w1, h1, w2, h2;
  oled.getTextBounds(MSG_BLE_CONNECTING_L1, 0, 0, &x1, &y1, &w1, &h1);
  oled.getTextBounds(MSG_BLE_CONNECTING_L2, 0, 0, &x1, &y1, &w2, &h2);

  oled.setCursor((SCREEN_WIDTH - w1) / 2, 6);
  oled.print(MSG_BLE_CONNECTING_L1);
  oled.setCursor((SCREEN_WIDTH - w2) / 2, 20);
  oled.print(MSG_BLE_CONNECTING_L2);

  oled.display();
}

void displayBleConnected() {
  displayMessage(MSG_BLE_CONNECTED);
}

void displayBleSyncing() {
  displayMessage(MSG_BLE_SYNCING);
}

void displayBleSyncDone() {
  displayMessage(MSG_BLE_SYNC_DONE);
}

void displayBleError() {
  displayMessage(MSG_BLE_ERROR);
}

// ── Confetti / final score ──────────────────────────────────────────────────
struct Confetti { uint8_t x; int8_t y; uint8_t speed; };
static Confetti confettiParticles[CONFETTI_COUNT];
static bool confettiInited = false;

void displayFinalScoreReset() {
  confettiInited = false;
}

void displayFinalScore(uint16_t score) {
  if (!confettiInited) {
    for (uint8_t i = 0; i < CONFETTI_COUNT; i++) {
      confettiParticles[i].x     = (i * 11) % 128;
      confettiParticles[i].y     = (int8_t)((i * 7) % 32);
      confettiParticles[i].speed = 1 + (i % 2);
    }
    confettiInited = true;
  }

  oled.clearDisplay();

  // Animate and draw confetti pixels
  for (uint8_t i = 0; i < CONFETTI_COUNT; i++) {
    confettiParticles[i].y += confettiParticles[i].speed;
    if (confettiParticles[i].y >= 32) {
      confettiParticles[i].y = -2;
      confettiParticles[i].x = (confettiParticles[i].x * 37 + 17) % 128; // LCG: pseudo-random x-scatter on wrap
    }
    if (confettiParticles[i].y >= 0) {
      oled.drawPixel(confettiParticles[i].x,
                     (uint8_t)confettiParticles[i].y, SSD1306_WHITE);
    }
  }

  // "Great job!" label — textSize(1), centered at top
  oled.setTextSize(1);
  const char* label = MSG_FINAL_SCORE_LABEL;
  int16_t labelW = (int16_t)strlen(label) * 6;
  oled.setCursor((SCREEN_WIDTH - labelW) / 2, 2);
  oled.print(label);

  // Score — textSize(2) = 12x16 px per char, centered, with rect frame
  oled.setTextSize(2);
  char buf[7];
  snprintf(buf, sizeof(buf), "%um", score);
  int16_t scoreW = (int16_t)strlen(buf) * 12;
  int16_t scoreX = (SCREEN_WIDTH - scoreW) / 2;
  oled.setCursor(scoreX, 14);
  oled.print(buf);
  oled.drawRect(scoreX - 3, 12, scoreW + 6, 20, SSD1306_WHITE);

  oled.display();
}

void displayHistoryList(uint16_t scrollOffset) {
  oled.clearDisplay();
  oled.setTextSize(1);

  for (uint8_t row = 0; row < HISTORY_VISIBLE_ROWS; row++) {
    uint16_t idx = scrollOffset + row;
    if (idx < dayScoreHistoryCount) {
      char buf[HISTORY_BUF_SIZE];
      snprintf(buf, sizeof(buf), "%c #%u  L%u",
        row == 0 ? '>' : ' ',
        (unsigned)(idx + 1),
        dayScoreHistory[idx].level);
      oled.setCursor(0, MENU_ROW0_Y + row * MENU_ROW_SPACING);
      oled.print(buf);
    }
  }

  displayScrollTriangles(
    scrollOffset > 0,
    dayScoreHistoryCount > 1 && scrollOffset + HISTORY_VISIBLE_ROWS < (uint16_t)dayScoreHistoryCount
  );

  oled.display();
}


