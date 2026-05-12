#include "scoring.h"
#include "rtc_time.h"

const uint16_t levelToPoints[] = {
  LEVEL_POINTS_0,
  LEVEL_POINTS_1,
  LEVEL_POINTS_2,
  LEVEL_POINTS_3,
  LEVEL_POINTS_4,
  LEVEL_POINTS_5,
  LEVEL_POINTS_6,
  LEVEL_POINTS_7,
  LEVEL_POINTS_8,
  LEVEL_POINTS_9
};

uint16_t dayScoreCounter = 0;
uint8_t selectedLevel = LEVEL_INIT;

DayScoreEntry dayScoreHistory[MAX_HISTORY_ENTRIES];
uint8_t dayScoreHistoryCount = 0;

void submitLevel() {
  if (dayScoreHistoryCount < MAX_HISTORY_ENTRIES) {
    getTimestamp(dayScoreHistory[dayScoreHistoryCount].timestamp);
    dayScoreHistory[dayScoreHistoryCount].level = selectedLevel;
    dayScoreHistoryCount++;
  }

  uint16_t points = levelToPoints[selectedLevel];
  uint32_t newScore = (uint32_t)dayScoreCounter + points;
  dayScoreCounter = (newScore > DAY_SCORE_MAX) ? DAY_SCORE_MAX : (uint16_t)newScore;
}

void resetDay() {
  dayScoreCounter = 0;
  dayScoreHistoryCount = 0;
  selectedLevel = LEVEL_INIT;
}

void deleteHistoryEntry(uint8_t index) {
  if (index >= dayScoreHistoryCount) return;
  uint16_t points = levelToPoints[dayScoreHistory[index].level];
  dayScoreCounter = (dayScoreCounter >= points) ? dayScoreCounter - points : 0;
  for (uint8_t i = index; i < dayScoreHistoryCount - 1; i++) {
    dayScoreHistory[i] = dayScoreHistory[i + 1];
  }
  dayScoreHistoryCount--;
}

void clearHistory() {
  dayScoreCounter      = 0;
  dayScoreHistoryCount = 0;
}
