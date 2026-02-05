#include "DashboardClient.h"

#include <WiFiS3.h>

#include "Log.h"
#include "Telemetry.h"
#include "WifiManager.h"

DashboardClient::DashboardClient(WifiManager& wifi, const char* host, int port)
  : _wifi(wifi), _host(host), _port(port) {}

static String toIsoTimestamp(const DateTime& now) {
  char buf[25];
  snprintf(buf, sizeof(buf), "%04d-%02d-%02dT%02d:%02d:%02d",
           now.year(), now.month(), now.day(),
           now.hour(), now.minute(), now.second());
  return String(buf);
}

bool DashboardClient::publish(const Telemetry& t) {
  if (!_wifi.ensureConnected()) return false;

  WiFiClient client;
  if (!client.connect(_host, _port)) {
    logInfo("DASH", "Connection failed");
    return false;
  }

  String json = "[{";
  json += "\"timestamp\": \"" + toIsoTimestamp(t.timestamp) + "\",";
  if (t.co2ppm == 0) {
    json += "\"valueCO2\": null,";
  } else {
    json += "\"valueCO2\": " + String(t.co2ppm) + ",";
  }
  json += "\"valueTemp\": " + String(t.tempC) + ",";
  json += "\"valueHum\": " + String(t.humPct) + ",";
  json += "\"climStatus\": " + String(t.climOn ? "true" : "false");
  json += "}]";

  client.println("PUT /arduino/publish HTTP/1.1");
  client.println("Host: " + String(_host));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(json.length());
  client.println("Connection: close");
  client.println();
  client.print(json);

  logInfo("DASH", "Payload sent : ");
  logInfo("DASH", String(_host) + String(":") + String(_port) + "/arduino/publish HTTP/1.1 PUT" + String(" ") + json);

  unsigned long startWait = millis();
  while (client.connected() && !client.available() && millis() - startWait < 2000) {
    delay(10);
  }

  if (client.available()) {
    String statusLine = client.readStringUntil('\n');
    statusLine.trim();
    logInfo("DASH", client.readString());
  } else {
    logInfo("DASH", "No response (timeout)");
  }

  client.stop();
  return true;
}

