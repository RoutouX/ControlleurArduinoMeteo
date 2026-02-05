#pragma once

#include <Arduino.h>

#include <AM2302-Sensor.h>

struct SensorMedian {
  float tempC;
  float humPct;
};

class SensorSampler {
public:
  SensorSampler(AM2302::AM2302_Sensor& sensor, unsigned long periodMs, int samplesPerMedian);

  // Returns true when a new median value is produced.
  bool tick(unsigned long nowMs, SensorMedian& out);

  int lastStatus() const { return _lastStatus; }

private:
  AM2302::AM2302_Sensor& _sensor;
  const unsigned long _periodMs;
  int _samplesPerMedian;

  unsigned long _lastSampleMs = 0;
  int _lastStatus = 255;

  int _count = 0;
  float _temps[16] = {0}; // supports up to 16 samples/median
  float _hums[16] = {0};
};
