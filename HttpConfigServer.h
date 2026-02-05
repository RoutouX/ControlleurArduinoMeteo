#pragma once

#include <Arduino.h>
#include <WiFiS3.h>

class RuntimeConfig;

class HttpConfigServer {
public:
  HttpConfigServer(RuntimeConfig& cfg, uint16_t port);

  void begin();
  void tick();

private:
  void handleClient_(WiFiClient& client);
  void handlePatchTemp_(const String& body, WiFiClient& client);
  void handlePatchHours_(const String& body, WiFiClient& client);

private:
  RuntimeConfig& _cfg;
  WiFiServer _server;
};

