#include "RuntimeConfig.h"

RuntimeConfig::RuntimeConfig(float tempMinDefault, float tempMaxDefault)
  : _tempMinC(tempMinDefault), _tempMaxC(tempMaxDefault) {
  for (int i = 0; i < 7; i++) {
    _schedule[i] = DaySchedule{0, 24 * 60};
  }
}

DaySchedule RuntimeConfig::scheduleForDow(int dow) const {
  if (dow < 0 || dow > 6) return DaySchedule{0, 0};
  return _schedule[dow];
}

void RuntimeConfig::setScheduleForDow(int dow, DaySchedule s) {
  if (dow < 0 || dow > 6) return;
  _schedule[dow] = s;
}

bool RuntimeConfig::isWithinSchedule(const DateTime& now) const {
  int dow = now.dayOfTheWeek(); // 0 = Sunday
  DaySchedule d = scheduleForDow(dow);
  int minutesNow = now.hour() * 60 + now.minute();
  return minutesNow >= d.startMin && minutesNow < d.endMin;
}

