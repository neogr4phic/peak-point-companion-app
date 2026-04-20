#ifndef SCORING_H
#define SCORING_H

#include <Arduino.h>

#define MAX_HISTORY_ENTRIES 100
#define MAX_DAYS 30

struct DayScoreEntry {
  char timestamp[9]; // "HH:MM:SS"
  uint8_t level;
};

struct DayEntry {
  char date[11]; // "dd.mm.yyyy"
  uint16_t score;
  DayScoreEntry history[MAX_HISTORY_ENTRIES];
  uint16_t historyCount;
};

extern const uint16_t levelToPoints[];

extern uint16_t dayScoreCounter;
extern uint8_t selectedLevel;

extern DayScoreEntry dayScoreHistory[];
extern uint8_t dayScoreHistoryCount;

extern DayEntry daysHistory[];
extern uint16_t daysHistoryCount;

void submitLevel();
void finishDay();

#endif
