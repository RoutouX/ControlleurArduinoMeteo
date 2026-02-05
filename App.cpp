#include "App.h"

#include <SPI.h>
#include <SD.h>

#include "Indicators.h"
#include "Leds.h"
#include "Log.h"
#include "Music.h"
#include "config.h"

App::App()
  : _cfg(TEMP_MIN, TEMP_MAX),
    _wifi(WIFI_SSID, WIFI_PASS, WifiManager::StaticIpConfig{localIP, dns, gateway, subnet}),
    _http(_cfg, (uint16_t)HTTP_PORT),
    _climClient(_wifi, CLIM_HOST, CLIM_PORT),
    _climate(_cfg, _climClient, TEMP_HYST),
    _dashboard(_wifi, DASHBOARD_HOST, DASHBOARD_PORT),
    _sensor(am2302, SENSOR_PERIOD_MS, SENSOR_MEDIAN_SAMPLES),
    _mute(pinButton, MUTE_MS),
    _ble(),
    _bleTelemetry(leds, 1),
    _console(_cfg, _climate, _ble, _bleTelemetry) {}

void App::fatalHalt_(const char* reason) {
  logInfo("FATAL", reason);
  leds.setColorRGB(0, 255, 0, 0);
  leds.setColorRGB(1, 255, 0, 0);
  while (true) {
    delay(1000);
  }
}

void App::begin() {
  Serial.begin(115200);
  while (!Serial) {}
  _console.begin();

  // Publish immediately on first telemetry-ready cycle.
  _lastPublishMs = millis() - DASHBOARD_PUBLISH_PERIOD_MS;

  pinMode(pinBuzzer, OUTPUT);
  player.begin(tempo);

  pinMode(pinBlink, OUTPUT);
  digitalWrite(pinBlink, LOW);

  _mute.begin();

  if (!rtc.begin()) {
    logInfo("HW", "RTC not found");
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  if (!SD.begin(pinSD)) {
    logInfo("HW", "SD card not detected");
  }

  am2302.begin();

  leds.setColorRGB(0, 255, 255, 255);
  leds.setColorRGB(1, 255, 255, 255);

  BleManager::Config bleCfg;
  bleCfg.targetName = BLE_TARGET_NAME;
  bleCfg.serviceUuid = BLE_SERVICE_UUID;
  bleCfg.payloadUuid = BLE_PAYLOAD_UUID;
  bleCfg.ackUuid = BLE_ACK_UUID;
  bleCfg.reconnectIntervalMs = BLE_RECONNECT_INTERVAL_MS;

  _bleTelemetry.begin(_ble);
  if (!_ble.begin(bleCfg)) {
    logInfo("BLE", "Init failed");
    fatalHalt_("BLE init failed");
  }

  startupAnimation(leds);
  leds.setColorRGB(0, 255, 255, 255);
  leds.setColorRGB(1, 255, 255, 255);

  logInfo("APP", "Station ready");

  _wifi.ensureConnected();
  _http.begin();
}

void App::tick() {
  unsigned long nowMs = millis();

  _console.tick();
  _ble.tick();
  _wifi.ensureConnected();
  _http.tick();

  _mute.tick(nowMs, player);
  bool isMuted = _mute.isMuted(nowMs);

  SensorMedian median{};
  if (_sensor.tick(nowMs, median)) {
    DateTime now = rtc.now();

    logInfo("SENSOR", String("Temp=") + median.tempC + "C Hum=" + median.humPct + "%");
    setTemperatureColor(leds, 0, median.tempC, _cfg.tempMinC(), _cfg.tempMaxC());

    _lastTempC = median.tempC;
    _lastHumPct = median.humPct;
    _hasSensorMedian = true;

    _climate.tick(now, median.tempC);
  }

  if (_hasSensorMedian && (nowMs - _lastPublishMs >= DASHBOARD_PUBLISH_PERIOD_MS)) {
    _lastPublishMs = nowMs; // throttle attempts even when publish fails

    Telemetry t{};
    t.timestamp = rtc.now();
    t.tempC = _lastTempC;
    t.humPct = _lastHumPct;
    t.co2ppm = _bleTelemetry.co2ppm();
    t.climOn = _climate.isOn();
    _dashboard.publish(t);
  }

  alarm(isMuted, (float)_bleTelemetry.co2ppm(), co2_buzz, nowMs);
}
