#include "rtc_time.h"

static RTC_Millis rtc;

void rtcTimeInit() {
  rtc.begin(DateTime(F(__DATE__), F(__TIME__)));
}

void getTimestamp(char* buffer) {
  DateTime now = rtc.now();
  snprintf(buffer, 9, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
}
