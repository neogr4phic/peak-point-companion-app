#ifndef RTC_TIME_H
#define RTC_TIME_H

#include <Arduino.h>
#include <RTClib.h>

void rtcTimeInit();
void getTimestamp(char* buffer);
void getDateString(char* buffer);

#endif
