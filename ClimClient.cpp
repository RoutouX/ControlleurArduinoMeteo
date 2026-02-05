#include "ClimClient.h"

#include <WiFiS3.h>

#include "Log.h"
#include "WifiManager.h"

ClimClient::ClimClient(WifiManager& wifi, const char* host, int port)
  : _wifi(wifi), _host(host), _port(port) {}

bool ClimClient::setTemp(float targetC) {
  char target[8];
  dtostrf(targetC, 0, 1, target);
  return sendCommand("TEMP", target);
}

bool ClimClient::sendCommand(const char* cmd, const char* value) {
  if (!_wifi.ensureConnected()) {
    logInfo("CLIM", "WiFi unavailable");
    return false;
  }

  WiFiClient client;
  if (!client.connect(_host, _port)) {
    logInfo("CLIM", "AC connection failed");
    return false;
  }

  String url = "/clim?command=";
  url += cmd;
  if (value) {
    url += "&value=";
    url += value;
  }

  client.print(String("GET ") + url + " HTTP/1.1\r\nHost: " + _host + "\r\nConnection: close\r\n\r\n");
  client.stop();
  return true;
}

