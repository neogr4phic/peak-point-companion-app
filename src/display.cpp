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
