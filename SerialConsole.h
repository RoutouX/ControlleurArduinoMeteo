#pragma once

#include <Arduino.h>

class BleManager;
class BleTelemetry;
class ClimateController;
class RuntimeConfig;

class SerialConsole {
public:
  SerialConsole(RuntimeConfig& cfg, ClimateController& climate, BleManager& ble, BleTelemetry& bleTel);

  void begin();
  void tick();

private:
  void handleLine_(char* line);

  void cmdHelp_();
  void cmdConfig_();
  void cmdStatus_();
  void cmdSetTempMin_(const char* arg);
  void cmdSetTempMax_(const char* arg);

private:
  RuntimeConfig& _cfg;
  ClimateController& _climate;
  BleManager& _ble;
  BleTelemetry& _bleTel;

  char _buf[96] = {0};
  size_t _len = 0;
};

