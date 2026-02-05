#pragma once

#include <Arduino.h>

class WifiManager;

class ClimClient {
public:
  ClimClient(WifiManager& wifi, const char* host, int port);

  bool sendCommand(const char* cmd, const char* value = nullptr);

  bool setOn() { return sendCommand("ON"); }
  bool setOff() { return sendCommand("OFF"); }
  bool setTemp(float targetC);

private:
  WifiManager& _wifi;
  const char* _host;
  int _port;
};

