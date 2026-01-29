#include "BLE.h"
#include <Arduino.h>
#include <cstring>

bool BleManager::begin(const Config& cfg) {
  _cfg = cfg;

  if (!_cfg.targetName || !_cfg.serviceUuid || !_cfg.payloadUuid || !_cfg.ackUuid) {
    Serial.println("[BLE] Config invalid (missing targetName/serviceUuid/payloadUuid/ackUuid)");
    return false;
  }

  if (!BLE.begin()) {
    Serial.println("[BLE] BLE.begin() failed");
    return false;
  }

  Serial.println("[BLE] BLE ready");
  resetToDisconnected_();
  return true;
}

void BleManager::tick() {
  BLE.poll();

  switch (_state) {
    case State::DISCONNECTED:
      tryReconnect_();
      break;
    case State::SCANNING:
      scanStep_();
      break;
    case State::CONNECTING:
      connectStep_();
      break;
    case State::CONNECTED:
      readStep_();
      checkDisconnect_();
      break;
  }
}

void BleManager::tryReconnect_() {
  if (millis() - _lastReconnectAttempt < _cfg.reconnectIntervalMs) return;

  _lastReconnectAttempt = millis();
  Serial.print("[BLE] Scanning for: ");
  Serial.println(_cfg.targetName);

  BLE.stopScan();
  BLE.scanForName(_cfg.targetName);
  _state = State::SCANNING;
}

void BleManager::scanStep_() {
  BLEDevice found = BLE.available();
  if (!found) return;

  Serial.print("[BLE] Found: ");
  Serial.println(found.localName());

  if (found.localName() == _cfg.targetName) {
    BLE.stopScan();
    _peripheral = found;
    _state = State::CONNECTING;
  }
}

void BleManager::connectStep_() {
  Serial.println("[BLE] Connecting...");

  if (!_peripheral.connect()) {
    Serial.println("[BLE] Connect failed");
    resetToDisconnected_();
    return;
  }

  Serial.println("[BLE] Connected, discovering attributes...");
  if (!_peripheral.discoverAttributes()) {
    Serial.println("[BLE] discoverAttributes failed");
    _peripheral.disconnect();
    resetToDisconnected_();
    return;
  }

  BLEService svc = _peripheral.service(_cfg.serviceUuid);
  if (!svc) {
    Serial.println("[BLE] Service not found");
    _peripheral.disconnect();
    resetToDisconnected_();
    return;
  }

  _payload = svc.characteristic(_cfg.payloadUuid);
  if (!_payload) {
    Serial.println("[BLE] Payload characteristic not found");
    _peripheral.disconnect();
    resetToDisconnected_();
    return;
  }

  _ack = svc.characteristic(_cfg.ackUuid);
  if (!_ack) {
    Serial.println("[BLE] ACK characteristic not found");
    _peripheral.disconnect();
    resetToDisconnected_();
    return;
  }

  if (_payload.canSubscribe()) {
    if (_payload.subscribe()) {
      Serial.println("[BLE] Subscribed to payload notifications/indications");
    } else {
      Serial.println("[BLE] Subscribe failed (still connected)");
    }
  } else {
    Serial.println("[BLE] Payload cannot subscribe()");
  }

  _state = State::CONNECTED;
  Serial.println("[BLE] READY");
  if (_onConnected) _onConnected();
}

void BleManager::readStep_() {
  if (!_payload) return;

  if (_payload.valueUpdated()) {
    uint8_t buf[96];
    int len = _payload.readValue(buf, sizeof(buf));
    if (len > 0 && _onData) _onData(buf, (size_t)len);
  }
}

void BleManager::checkDisconnect_() {
  if (!_peripheral.connected()) {
    Serial.println("[BLE] Disconnected");
    if (_onDisconnected) _onDisconnected();
    resetToDisconnected_();
  }
}

void BleManager::resetToDisconnected_() {
  BLE.stopScan();
  _state = State::DISCONNECTED;
}

bool BleManager::writePayload(const uint8_t* data, size_t len) {
  if (_state != State::CONNECTED) return false;
  if (!_payload) return false;

  // ArduinoBLE (central) n'a pas canWriteWithoutResponse() selon versions
  if (!_payload.canWrite()) return false;

  return _payload.writeValue(data, (int)len);
}

bool BleManager::writePayloadString(const char* s) {
  if (!s) return false;
  return writePayload((const uint8_t*)s, strlen(s));
}

bool BleManager::writeAckU32(uint32_t seq) {
  if (_state != State::CONNECTED) return false;
  if (!_ack) return false;

  // idem : pas de canWriteWithoutResponse() -> on teste canWrite()
  if (!_ack.canWrite()) return false;

  return _ack.writeValue((uint8_t*)&seq, sizeof(seq));
}
