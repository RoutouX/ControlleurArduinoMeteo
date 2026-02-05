#include "BleTelemetry.h"

#include <cstring>

#include <ChainableLED.h>

#include "BLE.h"
#include "Indicators.h"
#include "Log.h"

BleTelemetry* BleTelemetry::s_self = nullptr;

BleTelemetry::BleTelemetry(ChainableLED& leds, int eco2LedIndex)
  : _leds(leds), _eco2LedIndex(eco2LedIndex) {}

void BleTelemetry::begin(BleManager& ble) {
  s_self = this;
  _ble = &ble;
  ble.onConnected(onConnectedThunk_);
  ble.onDisconnected(onDisconnectedThunk_);
  ble.onData(onDataThunk_);
}

void BleTelemetry::onConnectedThunk_() {
  if (s_self) s_self->onConnected_();
}

void BleTelemetry::onDisconnectedThunk_() {
  if (s_self) s_self->onDisconnected_();
}

void BleTelemetry::onDataThunk_(const uint8_t* data, size_t len) {
  if (s_self) s_self->onData_(data, len);
}

void BleTelemetry::onConnected_() {
  logInfo("BLE", "Connected");
}

void BleTelemetry::onDisconnected_() {
  logInfo("BLE", "Disconnected");
}

void BleTelemetry::onData_(const uint8_t* data, size_t len) {
  static char line[96];

  size_t n = (len < sizeof(line) - 1) ? len : sizeof(line) - 1;
  memcpy(line, data, n);
  line[n] = '\0';

  logDebug("BLE", String("Payload: ") + line);

  // Format: seq;timestamp;datetime;value
  char* token;
  char* rest = line;

  token = strtok_r(rest, ";", &rest);
  if (!token) return;
  _seq = strtoul(token, nullptr, 10);

  token = strtok_r(nullptr, ";", &rest);
  if (!token) return;
  _timestamp = strtoul(token, nullptr, 10);

  token = strtok_r(nullptr, ";", &rest);
  if (!token) return;
  strncpy(_datetime, token, sizeof(_datetime) - 1);
  _datetime[sizeof(_datetime) - 1] = '\0';

  token = strtok_r(nullptr, ";", &rest);
  if (!token) return;
  _co2ppm = atoi(token);

  _payloadValid = true;
  logInfo("BLE", String("eco2=") + _co2ppm);

  if (_ble) {
    bool ok = _ble->writeAckU32(_seq);
    logInfo("BLE", String("ACK ") + _seq + (ok ? " OK" : " FAIL"));
  }

  setAirQualityColor(_leds, _eco2LedIndex, (uint16_t)_co2ppm);
}
