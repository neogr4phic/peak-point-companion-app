#ifndef SCORING_H
#define SCORING_H

#include <Arduino.h>
#include "config.h"

struct DayScoreEntry {
  char timestamp[9]; // "HH:MM:SS"
  uint8_t level;
};

extern uint16_t dayScoreCounter;
extern uint8_t selectedLevel;

extern DayScoreEntry dayScoreHistory[];
extern uint8_t dayScoreHistoryCount;

void submitLevel();
void resetDay();

#endif
