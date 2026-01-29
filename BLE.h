#pragma once
#include <ArduinoBLE.h>

class BleManager {
public:
  enum class State : uint8_t {
    DISCONNECTED,
    SCANNING,
    CONNECTING,
    CONNECTED
  };

  struct Config {
    const char* targetName  = "StationAir";
    const char* serviceUuid = "181A";
    const char* payloadUuid = "3006";
    const char* ackUuid     = "3007";
    unsigned long reconnectIntervalMs = 60000;
  };

  using OnConnectedFn = void (*)();
  using OnDisconnectedFn = void (*)();
  using OnDataFn = void (*)(const uint8_t* data, size_t len);

  bool begin(const Config& cfg);

  void tick();

  State state() const { return _state; }
  bool connected() const { return _state == State::CONNECTED; }

  bool writePayload(const uint8_t* data, size_t len);
  bool writePayloadString(const char* s);

  bool writeAckU32(uint32_t seq);

  void onConnected(OnConnectedFn fn) { _onConnected = fn; }
  void onDisconnected(OnDisconnectedFn fn) { _onDisconnected = fn; }
  void onData(OnDataFn fn) { _onData = fn; }

private:
  void tryReconnect_();
  void scanStep_();
  void connectStep_();
  void readStep_();
  void checkDisconnect_();
  void resetToDisconnected_();

private:
  Config _cfg{};
  State _state = State::DISCONNECTED;

  BLEDevice _peripheral;
  BLECharacteristic _payload;
  BLECharacteristic _ack;

  unsigned long _lastReconnectAttempt = 0;

  OnConnectedFn _onConnected = nullptr;
  OnDisconnectedFn _onDisconnected = nullptr;
  OnDataFn _onData = nullptr;
};
