// Config.cpp
#include "config.h"

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
const float ALARM_TEMP = 24.8f;

unsigned long lastSensorMs = 0;

// --------- MUTE BUTTON (2 minutes) ----------
const unsigned long MUTE_MS = 120000UL; // 2 minutes
unsigned long muteUntilMs = 0;

// Delais mesure

extern const unsigned int nb_mesure = 10;
extern const unsigned int delais_mesure;
const unsigned long SENSOR_PERIOD_MS = 2010UL;



