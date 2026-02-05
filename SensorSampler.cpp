#include "SensorSampler.h"

#include "Log.h"
#include "Math.h"

SensorSampler::SensorSampler(AM2302::AM2302_Sensor& sensor, unsigned long periodMs, int samplesPerMedian)
  : _sensor(sensor), _periodMs(periodMs), _samplesPerMedian(samplesPerMedian) {
  int maxN = (int)(sizeof(_temps) / sizeof(_temps[0]));
  if (_samplesPerMedian > maxN) {
    _samplesPerMedian = maxN;
    logWarn("SENSOR", "samplesPerMedian too big, clamping to 16");
  }
  if (_samplesPerMedian <= 0) _samplesPerMedian = 1;
}

bool SensorSampler::tick(unsigned long nowMs, SensorMedian& out) {
  if (nowMs - _lastSampleMs < _periodMs) return false;
  _lastSampleMs = nowMs;

  noInterrupts();
  _lastStatus = _sensor.read();
  interrupts();

  if (_lastStatus != 0) {
    logInfo("SENSOR", String("Error status=") + _lastStatus);
    return false;
  }

  float temp = _sensor.get_Temperature();
  float hum = _sensor.get_Humidity();

  int n = _samplesPerMedian;

  _temps[_count] = temp;
  _hums[_count] = hum;
  _count++;

  if (_count < n) return false;

  out.tempC = mediane(_temps, n);
  out.humPct = mediane(_hums, n);
  _count = 0;
  return true;
}
