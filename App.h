#pragma once

#include "BLE.h"
#include "BleTelemetry.h"
#include "ClimClient.h"
#include "ClimateController.h"
#include "DashboardClient.h"
#include "HttpConfigServer.h"
#include "MuteController.h"
#include "RuntimeConfig.h"
#include "SensorSampler.h"
#include "SerialConsole.h"
#include "Telemetry.h"
#include "WifiManager.h"

class App {
public:
  App();

  void begin();
  void tick();

private:
  void fatalHalt_(const char* reason);

private:
  unsigned long _lastPublishMs = 0;
  float _lastTempC = NAN;
  float _lastHumPct = NAN;
  bool _hasSensorMedian = false;

  RuntimeConfig _cfg;
  WifiManager _wifi;
  HttpConfigServer _http;
  ClimClient _climClient;
  ClimateController _climate;
  DashboardClient _dashboard;
  SensorSampler _sensor;
  MuteController _mute;

  BleManager _ble;
  BleTelemetry _bleTelemetry;

  SerialConsole _console;
};
