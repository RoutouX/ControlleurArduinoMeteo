// Config.cpp
#include "config.h"
#include <WiFiS3.h>

// ----------------- HW PINS -----------------
const byte DHTPIN = 2;

const int pinBuzzer = 10;   // buzzer sur D10
const int pinSD     = 4;    // CS SD sur D4
const int pinButton = 5;    // module bouton sur D5

const int pinBlink  = LED_BUILTIN;

// ----------------- LEDs -----------------
ChainableLED leds(7, 8, NUM_LEDS);

// ----------------- DEVICES -----------------
AM2302::AM2302_Sensor am2302{ DHTPIN };
RTC_DS1307 rtc;

// ----------------- APP PARAMS -----------------
const float co2_buzz = 1000;
const int SENSOR_MEDIAN_SAMPLES = 10;
const unsigned long SENSOR_PERIOD_MS = 2010UL;
const unsigned long DASHBOARD_PUBLISH_PERIOD_MS = 300000UL; // 5 minutes

// --------- MUTE BUTTON (2 minutes) ----------
const unsigned long MUTE_MS = 120000UL; // 2 minutes

// --------- WiFi static IP + credentials ----------
const char* WIFI_SSID = "A4_IOT_CESI";
const char* WIFI_PASS = "WrVqBaGJRbRVh857KQzEmRtcnToVvo8kBh7VhDd8MjRUmHpEYS";
IPAddress localIP(192, 168, 100, 170);
IPAddress gateway(192, 168, 100, 252);
IPAddress dns(192, 168, 100, 252);
IPAddress subnet(255, 255, 255, 0);

// --------- Clim / Dashboard / HTTP ----------
const char* CLIM_HOST = "192.168.100.162";
const int   CLIM_PORT = 8080;
const float TEMP_MIN = 21.0;
const float TEMP_MAX = 24.0;
const float TEMP_HYST = 0.4;
const char* DASHBOARD_HOST = "192.168.100.10";
const int   DASHBOARD_PORT = 800;
const int   HTTP_PORT = 80;

// --------- BLE CONFIG ----------
const char* BLE_TARGET_NAME = "StationAir";
const char* BLE_SERVICE_UUID = "181A";
const char* BLE_PAYLOAD_UUID = "3006";
const char* BLE_ACK_UUID = "3007";
const unsigned long BLE_RECONNECT_INTERVAL_MS = 60000UL;
