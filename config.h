// Config.h
#pragma once
#include <Arduino.h>

#include <AM2302-Sensor.h>
#include <ChainableLED.h>
#include "RTClib.h"

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
extern const float ALARM_TEMP;
extern bool alarmEnabled;

extern unsigned long lastSensorMs;

// --------- MUTE BUTTON (2 minutes) ----------
extern const unsigned long MUTE_MS;
extern unsigned long muteUntilMs;

// --------- Capteur----------------------------
extern const unsigned int nb_mesure;
extern const unsigned int delais_mesure;

