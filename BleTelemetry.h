#pragma once

#include <Arduino.h>

class BleManager;
class ChainableLED;

class BleTelemetry {
public:
  BleTelemetry(ChainableLED& leds, int eco2LedIndex);

  void begin(BleManager& ble);

  int co2ppm() const { return _co2ppm; }
  bool hasPayload() const { return _payloadValid; }

private:
  void onConnected_();
  void onDisconnected_();
  void onData_(const uint8_t* data, size_t len);

  static void onConnectedThunk_();
  static void onDisconnectedThunk_();
  static void onDataThunk_(const uint8_t* data, size_t len);

private:
  static BleTelemetry* s_self;

  BleManager* _ble = nullptr;
  ChainableLED& _leds;
  int _eco2LedIndex;

  uint32_t _seq = 0;
  uint32_t _timestamp = 0;
  char _datetime[24] = {0};

  int _co2ppm = 0;
  bool _payloadValid = false;
};

