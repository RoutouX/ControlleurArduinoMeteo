#include "App.h"

#include <SPI.h>
#include <SD.h>

#include "Indicators.h"
#include "Leds.h"
#include "Log.h"
#include "Music.h"
#include "config.h"

static void logDashboardCountdownMs(unsigned long remainingMs) {
  unsigned long totalSeconds = remainingMs / 1000UL;
  unsigned long minutes = totalSeconds / 60UL;
  unsigned long seconds = totalSeconds % 60UL;

  char buf[64];
  snprintf(buf, sizeof(buf), "Prochain envoi dashboard dans %lum%02lus", minutes, seconds);
  logInfo("DASH", buf);
}

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
  _nextDashboardPublishMs = 0;
  _lastPublishCountdownLogMs = millis();

  pinMode(pinBuzzer, OUTPUT);
  player.begin(tempo);

  pinMode(pinBlink, OUTPUT);
  digitalWrite(pinBlink, LOW);

  _mute.begin();

  bool rtcOk = rtc.begin();
  if (!rtcOk) {
    logInfo("HW", "RTC not found");
  } else {
    bool rtcRunning = rtc.isrunning();
    DateTime buildTime(F(__DATE__), F(__TIME__));
    DateTime rtcNow = rtc.now();

    // IMPORTANT: do not reset the RTC time on every boot.
    // Only initialize it when the clock isn't running, or when it looks
    // clearly older than the firmware build time (ex: after a power loss).
    const uint32_t kBuildTimeSlackSeconds = 10UL * 60UL;
    bool rtcClearlyBehindBuild =
      rtcNow.unixtime() + kBuildTimeSlackSeconds < buildTime.unixtime();

    if (!rtcRunning || rtcClearlyBehindBuild) {
      logInfo("HW", "RTC not set, setting time to build time");
      rtc.adjust(buildTime);
    }
  }

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
    if (_nextDashboardPublishMs == 0) _nextDashboardPublishMs = nowMs;

    _climate.tick(now, median.tempC);
  }

  if (_hasSensorMedian && _nextDashboardPublishMs != 0 && nowMs >= _nextDashboardPublishMs) {
    Telemetry t{};
    t.timestamp = rtc.now();
    t.tempC = _lastTempC;
    t.humPct = _lastHumPct;
    t.co2ppm = _bleTelemetry.co2ppm();
    t.climOn = _climate.isOn();
    bool ok = _dashboard.publish(t);

    _lastPublishCountdownLogMs = nowMs;
    unsigned long nextDelayMs = ok ? DASHBOARD_PUBLISH_PERIOD_MS : DASHBOARD_RETRY_PERIOD_MS;
    _nextDashboardPublishMs = nowMs + nextDelayMs;
    if (!ok) {
      logWarn("DASH", String("Publish failed, retry in ") + (nextDelayMs / 1000UL) + "s");
    }
    logDashboardCountdownMs(nextDelayMs);
  } else if (_hasSensorMedian) {
    static const unsigned long kCountdownLogEveryMs = 30000UL;
    if (nowMs - _lastPublishCountdownLogMs >= kCountdownLogEveryMs) {
      _lastPublishCountdownLogMs = nowMs;

      if (_nextDashboardPublishMs != 0 && _nextDashboardPublishMs > nowMs) {
        logDashboardCountdownMs(_nextDashboardPublishMs - nowMs);
      }
    }
  }

  alarm(isMuted, (float)_bleTelemetry.co2ppm(), co2_buzz, nowMs);
}
