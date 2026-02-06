#include "SerialConsole.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <WiFiS3.h>

#include "BLE.h"
#include "BleTelemetry.h"
#include "ClimateController.h"
#include "RuntimeConfig.h"
#include "WifiManager.h"
#include "config.h"

static const char* dayName(int dow) {
  switch (dow) {
    case 0: return "sunday";
    case 1: return "monday";
    case 2: return "tuesday";
    case 3: return "wednesday";
    case 4: return "thursday";
    case 5: return "friday";
    case 6: return "saturday";
    default: return "?";
  }
}

static void print2d(int v) {
  if (v < 10) Serial.print('0');
  Serial.print(v);
}

static void printTimeMm(int minutes) {
  if (minutes < 0) minutes = 0;
  int hh = minutes / 60;
  int mm = minutes % 60;
  if (hh > 23) hh = 23;
  if (mm > 59) mm = 59;
  print2d(hh);
  Serial.print(':');
  print2d(mm);
}

static bool parseFloatArg(const char* s, float& out) {
  if (!s) return false;
  while (*s && isspace((unsigned char)*s)) s++;
  if (!*s) return false;

  bool hasDigit = false;
  for (const char* p = s; *p; p++) {
    if (isdigit((unsigned char)*p)) {
      hasDigit = true;
      break;
    }
    if (*p == '-' || *p == '+' || *p == '.' || isspace((unsigned char)*p)) continue;
    break;
  }
  if (!hasDigit) return false;

  out = (float)atof(s);
  return true;
}

static const char* bleStateToStr(BleManager::State s) {
  switch (s) {
    case BleManager::State::DISCONNECTED: return "DISCONNECTED";
    case BleManager::State::SCANNING: return "SCANNING";
    case BleManager::State::CONNECTING: return "CONNECTING";
    case BleManager::State::CONNECTED: return "CONNECTED";
    default: return "UNKNOWN";
  }
}

SerialConsole::SerialConsole(RuntimeConfig& cfg, ClimateController& climate, BleManager& ble, BleTelemetry& bleTel)
  : _cfg(cfg), _climate(climate), _ble(ble), _bleTel(bleTel) {}

void SerialConsole::begin() {
  Serial.println("Type /help for commands");
}

void SerialConsole::tick() {
  while (Serial.available() > 0) {
    int v = Serial.read();
    if (v < 0) break;
    char c = (char)v;

    if (c == '\r' || c == '\n') {
      _buf[_len] = '\0';
      handleLine_(_buf);
      _len = 0;
      continue;
    }

    if (_len < sizeof(_buf) - 1) {
      _buf[_len++] = c;
    }
  }
}

void SerialConsole::handleLine_(char* line) {
  if (!line) return;

  // Trim leading spaces
  while (*line && isspace((unsigned char)*line)) line++;
  if (!*line) return;

  char* save = nullptr;
  char* cmd = strtok_r(line, " ", &save);
  char* arg1 = strtok_r(nullptr, " ", &save);

  // Support "/setTempMin=21.5" or "/setTempMin:21.5"
  if (cmd && !arg1) {
    char* sep = strchr(cmd, '=');
    if (!sep) sep = strchr(cmd, ':');
    if (sep) {
      *sep = '\0';
      arg1 = sep + 1;
    }
  }

  if (!cmd) return;

  if (strcmp(cmd, "/help") == 0 || strcmp(cmd, "help") == 0 || strcmp(cmd, "/?") == 0) {
    cmdHelp_();
    return;
  }
  if (strcmp(cmd, "/config") == 0) {
    cmdConfig_();
    return;
  }
  if (strcmp(cmd, "/status") == 0) {
    cmdStatus_();
    return;
  }
  if (strcmp(cmd, "/setTempMin") == 0) {
    cmdSetTempMin_(arg1);
    return;
  }
  if (strcmp(cmd, "/setTempMax") == 0) {
    cmdSetTempMax_(arg1);
    return;
  }

  Serial.print("Unknown command: ");
  Serial.println(cmd);
  Serial.println("Type /help");
}

void SerialConsole::cmdHelp_() {
  Serial.println("Commands:");
  Serial.println("  /help                Show this help");
  Serial.println("  /config              Show build + runtime config");
  Serial.println("  /status              Show current status (wifi/ble/clim/co2/schedule)");
  Serial.println("  /setTempMin <C>      Set runtime temp min (ex: /setTempMin 21.5)");
  Serial.println("  /setTempMax <C>      Set runtime temp max (ex: /setTempMax 24.0)");
}

void SerialConsole::cmdStatus_() {
  DateTime now = rtc.now();

  Serial.print("Time: ");
  print2d(now.hour()); Serial.print(':'); print2d(now.minute()); Serial.print(':'); print2d(now.second());
  Serial.println();

  Serial.print("WiFi: ");
  Serial.print(WifiManager::statusToStr(WiFi.status()));
  Serial.print(" IP=");
  Serial.println(WiFi.localIP());

  Serial.print("BLE: ");
  Serial.print(bleStateToStr(_ble.state()));
  Serial.print(" co2=");
  Serial.println(_bleTel.co2ppm());

  Serial.print("Clim: ");
  Serial.print(_climate.isOn() ? "ON" : "OFF");
  Serial.print(" schedule=");
  Serial.println(_cfg.isWithinSchedule(now) ? "IN" : "OUT");

  Serial.print("Temp range: ");
  Serial.print(_cfg.tempMinC());
  Serial.print(" .. ");
  Serial.println(_cfg.tempMaxC());
}

void SerialConsole::cmdConfig_() {
  Serial.println("=== Build config ===");

  Serial.print("WiFi SSID: ");
  Serial.println(WIFI_SSID);

  Serial.print("Static IP: ");
  Serial.println(localIP);
  Serial.print("Gateway:   ");
  Serial.println(gateway);
  Serial.print("DNS:       ");
  Serial.println(dns);
  Serial.print("Subnet:    ");
  Serial.println(subnet);

  Serial.print("Clim:      ");
  Serial.print(CLIM_HOST);
  Serial.print(":");
  Serial.println(CLIM_PORT);

  Serial.print("Dashboard: ");
  Serial.print(DASHBOARD_HOST);
  Serial.print(":");
  Serial.println(DASHBOARD_PORT);

  Serial.print("HTTP port: ");
  Serial.println(HTTP_PORT);

  Serial.print("Temp default: ");
  Serial.print(TEMP_MIN);
  Serial.print(" .. ");
  Serial.print(TEMP_MAX);
  Serial.print(" (hyst=");
  Serial.print(TEMP_HYST);
  Serial.println(")");

  Serial.print("Sensor period ms: ");
  Serial.println(SENSOR_PERIOD_MS);
  Serial.print("Publish period ms: ");
  Serial.println(DASHBOARD_PUBLISH_PERIOD_MS);
  Serial.print("Publish retry ms:  ");
  Serial.println(DASHBOARD_RETRY_PERIOD_MS);
  Serial.print("Median samples:   ");
  Serial.println(SENSOR_MEDIAN_SAMPLES);
  Serial.print("Mute ms:          ");
  Serial.println(MUTE_MS);

  Serial.print("BLE target: ");
  Serial.println(BLE_TARGET_NAME);
  Serial.print("BLE service: ");
  Serial.println(BLE_SERVICE_UUID);
  Serial.print("BLE payload: ");
  Serial.println(BLE_PAYLOAD_UUID);
  Serial.print("BLE ack:     ");
  Serial.println(BLE_ACK_UUID);
  Serial.print("BLE reconnect ms: ");
  Serial.println(BLE_RECONNECT_INTERVAL_MS);

#ifdef LOG_LEVEL
  Serial.print("LOG_LEVEL: ");
  Serial.println(LOG_LEVEL);
#endif

  Serial.println("=== Runtime config ===");
  Serial.print("Temp: ");
  Serial.print(_cfg.tempMinC());
  Serial.print(" .. ");
  Serial.println(_cfg.tempMaxC());

  Serial.println("Hours (dow 0=sunday):");
  for (int i = 0; i < 7; i++) {
    DaySchedule d = _cfg.scheduleForDow(i);
    Serial.print("  ");
    Serial.print(dayName(i));
    Serial.print(" ");
    printTimeMm(d.startMin);
    Serial.print("-");
    printTimeMm(d.endMin);
    Serial.println();
  }
}

void SerialConsole::cmdSetTempMin_(const char* arg) {
  float v = 0.0f;
  if (!parseFloatArg(arg, v)) {
    Serial.println("Usage: /setTempMin <C>");
    return;
  }

  if (v >= _cfg.tempMaxC()) {
    Serial.println("Error: min must be < max");
    return;
  }

  _cfg.setTempMinC(v);
  Serial.print("OK tempMin=");
  Serial.println(_cfg.tempMinC());
}

void SerialConsole::cmdSetTempMax_(const char* arg) {
  float v = 0.0f;
  if (!parseFloatArg(arg, v)) {
    Serial.println("Usage: /setTempMax <C>");
    return;
  }

  if (v <= _cfg.tempMinC()) {
    Serial.println("Error: max must be > min");
    return;
  }

  _cfg.setTempMaxC(v);
  Serial.print("OK tempMax=");
  Serial.println(_cfg.tempMaxC());
}
