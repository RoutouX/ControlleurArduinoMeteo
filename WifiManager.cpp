#include "WifiManager.h"

#include "Log.h"

WifiManager::WifiManager(const char* ssid, const char* pass, const StaticIpConfig& ipCfg)
  : _ssid(ssid), _pass(pass), _ipCfg(ipCfg) {}

const char* WifiManager::statusToStr(int s) {
  switch (s) {
    case WL_CONNECTED: return "CONNECTED";
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "NO_SSID";
    case WL_SCAN_COMPLETED: return "SCAN_DONE";
    case WL_CONNECT_FAILED: return "CONNECT_FAIL";
    case WL_CONNECTION_LOST: return "CONN_LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    case WL_NO_MODULE: return "NO_MODULE";
    default: return "UNKNOWN";
  }
}

bool WifiManager::ensureConnected() {
  static const unsigned long kRetryMs = 5000;

  if (WiFi.status() == WL_CONNECTED) return true;

  // Avoid spamming connect attempts
  if (millis() - _lastAttemptMs < kRetryMs) return false;
  _lastAttemptMs = millis();

  if (!_ssid || !_pass || strcmp(_ssid, "YOUR_WIFI") == 0) {
    logInfo("WIFI", "SSID/PASS not configured");
    return false;
  }

  logInfo("WIFI", String("Connecting to ") + _ssid);

  // Static IP must be configured before begin()
  WiFi.config(_ipCfg.localIp, _ipCfg.dns, _ipCfg.gateway, _ipCfg.subnet);
  WiFi.begin(_ssid, _pass);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(100);
  }

  if (WiFi.status() == WL_CONNECTED) {
    logInfo("WIFI", String("Connected IP ") + String(WiFi.localIP()));
    return true;
  }

  logInfo("WIFI", String("Connect failed status=") + statusToStr(WiFi.status()));
  logInfo("WIFI", String("Firmware=") + WiFi.firmwareVersion());
  return false;
}

