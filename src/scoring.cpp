#include "scoring.h"
#include "rtc_time.h"

const uint16_t levelToPoints[] = {0, 1, 3, 6, 15, 25, 45, 80, 130, 200};

uint16_t dayScoreCounter = 0;
uint8_t selectedLevel = 1;

DayScoreEntry dayScoreHistory[MAX_HISTORY_ENTRIES];
uint8_t dayScoreHistoryCount = 0;

DayEntry daysHistory[MAX_DAYS];
uint16_t daysHistoryCount = 0;

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
