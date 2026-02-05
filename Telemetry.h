#pragma once

#include <Arduino.h>
#include "RTClib.h"

struct Telemetry {
  DateTime timestamp;
  float tempC = NAN;
  float humPct = NAN;
  int co2ppm = 0;
  bool climOn = false;
};

