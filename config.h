// Config.h
#pragma once
#include <Arduino.h>

#include <AM2302-Sensor.h>
#include <ChainableLED.h>
#include "RTClib.h"
#include <WiFiS3.h>

// ----------------- HW PINS -----------------
extern const byte DHTPIN;

extern const int pinBuzzer;     // buzzer
extern const int pinSD;         // CS SD
extern const int pinButton;     // module bouton

extern const int pinBlink;      // LED blink (souvent LED_BUILTIN)

// ----------------- LEDs -----------------
#define NUM_LEDS 2
extern ChainableLED leds;

// ----------------- DEVICES -----------------
extern AM2302::AM2302_Sensor am2302;
extern RTC_DS1307 rtc;

// ----------------- APP PARAMS -----------------
extern const unsigned long SENSOR_PERIOD_MS;
extern const unsigned long DASHBOARD_PUBLISH_PERIOD_MS;
extern const float co2_buzz;
extern const int SENSOR_MEDIAN_SAMPLES;

// --------- MUTE BUTTON (2 minutes) ----------
extern const unsigned long MUTE_MS;

// --------- WiFi static IP --------------------
extern IPAddress localIP;
extern IPAddress gateway;
extern IPAddress dns;
extern IPAddress subnet;

// --------- BLE ----------------------------

// --------- APP CONFIG ---------
extern const char* WIFI_SSID;
extern const char* WIFI_PASS;
extern const char* CLIM_HOST;
extern const int   CLIM_PORT;
extern const float TEMP_MIN;
extern const float TEMP_MAX;
extern const float TEMP_HYST;
extern const char* DASHBOARD_HOST;
extern const int   DASHBOARD_PORT;
extern const int   HTTP_PORT;

// --------- BLE CONFIG ---------
extern const char* BLE_TARGET_NAME;
extern const char* BLE_SERVICE_UUID;
extern const char* BLE_PAYLOAD_UUID;
extern const char* BLE_ACK_UUID;
extern const unsigned long BLE_RECONNECT_INTERVAL_MS;

