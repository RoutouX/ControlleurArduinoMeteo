#pragma once

#include <Arduino.h>
#include <WiFiS3.h>

class WifiManager {
public:
  struct StaticIpConfig {
    IPAddress localIp;
    IPAddress dns;
    IPAddress gateway;
    IPAddress subnet;
  };

  WifiManager(const char* ssid, const char* pass, const StaticIpConfig& ipCfg);

  bool ensureConnected();

  static const char* statusToStr(int status);

private:
  const char* _ssid;
  const char* _pass;
  StaticIpConfig _ipCfg;

  unsigned long _lastAttemptMs = 0;
};

