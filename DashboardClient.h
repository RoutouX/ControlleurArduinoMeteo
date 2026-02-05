#pragma once

#include <Arduino.h>

class WifiManager;
struct Telemetry;

class DashboardClient {
public:
  DashboardClient(WifiManager& wifi, const char* host, int port);

  bool publish(const Telemetry& t);

private:
  WifiManager& _wifi;
  const char* _host;
  int _port;
};

