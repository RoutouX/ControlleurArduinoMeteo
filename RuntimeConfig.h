#pragma once

#include <Arduino.h>
#include "RTClib.h"

struct DaySchedule {
  int startMin;  // minutes from midnight, inclusive
  int endMin;    // minutes from midnight, exclusive
};

class RuntimeConfig {
public:
  RuntimeConfig(float tempMinDefault, float tempMaxDefault);

  float tempMinC() const { return _tempMinC; }
  float tempMaxC() const { return _tempMaxC; }

  void setTempMinC(float v) { _tempMinC = v; }
  void setTempMaxC(float v) { _tempMaxC = v; }
  void setTempRangeC(float minC, float maxC) {
    _tempMinC = minC;
    _tempMaxC = maxC;
  }

  DaySchedule scheduleForDow(int dow) const;
  void setScheduleForDow(int dow, DaySchedule s);

  bool isWithinSchedule(const DateTime& now) const;

private:
  float _tempMinC;
  float _tempMaxC;
  DaySchedule _schedule[7];
};

