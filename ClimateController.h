#pragma once

#include <Arduino.h>
#include "RTClib.h"

class ClimClient;
class RuntimeConfig;

class ClimateController {
public:
  ClimateController(RuntimeConfig& cfg, ClimClient& climClient, float hystC);

  void tick(const DateTime& now, float roomTempC);

  bool isOn() const { return _isOn; }

private:
  RuntimeConfig& _cfg;
  ClimClient& _clim;
  const float _hystC;

  bool _isOn = false;
};

