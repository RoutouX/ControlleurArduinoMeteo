#include <SPI.h>
#include <SD.h>
#include "Music.h"
#include "Math.h"
#include "config.h"
#include "Leds.h"
#include "PrintSD.h"
#include "RTClib.h"
#include <AM2302-Sensor.h>
#include <ChainableLED.h>




extern MusicPlayer player;

// =======================================================
//                     SENSOR STATE
// =======================================================
uint8_t lastStatus = 255;

unsigned int nb_mesure_c = 0;
float nb_mesure_temp_t[10] = {0};
float nb_mesure_humi_t[10] = {0};

float median_temp = NAN;
float median_humi = NAN;

// =======================================================
//                         SETUP
// =======================================================
void setup() {

  // Init terminal
  Serial.begin(115200);
  while (!Serial) {}

  // Init Buzzer
  pinMode(pinBuzzer, OUTPUT);
  player.begin(tempo);

  // Led RGB
  pinMode(pinBlink, OUTPUT);
  digitalWrite(pinBlink, LOW);

  // Button
  pinMode(pinButton, INPUT_PULLUP);

  // RTC
  if (!rtc.begin()) {
    Serial.println("ERREUR : RTC introuvable !");
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  // SD Card
  if (!SD.begin(pinSD)) {
    Serial.println("ERREUR : Carte SD non detectee !");
  }

  //Capteur
  am2302.begin();

  //End
  leds.setColorRGB(0, 255, 255, 255);
  leds.setColorRGB(1, 255, 255, 255);
  startupAnimation(leds);
  Serial.println("Station Meteo prete");
}

// =======================================================
//                          LOOP
// =======================================================
void loop() {
  unsigned long nowMs = millis();

  static bool stableState = HIGH;
  static bool lastRead = HIGH;
  static unsigned long lastChangeMs = 0;
  const unsigned long DEBOUNCE_MS = 30;

  bool readNow = digitalRead(pinButton);

  if (readNow != lastRead) {
    lastRead = readNow;
    lastChangeMs = nowMs;
  }

  if ((nowMs - lastChangeMs) > DEBOUNCE_MS && readNow != stableState) {
    stableState = readNow;
    if (stableState == LOW) {
      muteUntilMs = nowMs + MUTE_MS;
      player.stop();
      Serial.println("MUTE 2 minutes !");
    }
  }

  bool isMuted = (nowMs < muteUntilMs);

  if (nowMs - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = nowMs;

    auto status = am2302.read();
    lastStatus = status;

    if (status == 0) {
      DateTime now = rtc.now();
      float temp = am2302.get_Temperature();
      float humi  = am2302.get_Humidity();
      nb_mesure_temp_t[nb_mesure_c] = temp;
      nb_mesure_humi_t[nb_mesure_c] = humi;
      nb_mesure_c++;
      if(nb_mesure_c == 10){
        median_temp = mediane(nb_mesure_temp_t, 10);
        median_humi = mediane(nb_mesure_humi_t, 10);
        Serial.print(" -> Temp: "); Serial.print(median_temp);
        Serial.print(" C | Hum: "); Serial.print(median_humi);
        Serial.println(" %");


      // LED
      if (median_temp < 22.0) {
        leds.setColorRGB(0, 0, 0, 255);      // bleu
      } else if (median_temp <= ALARM_TEMP) {
        leds.setColorRGB(0, 0, 255, 0);      // vert
      } else {
        leds.setColorRGB(0, 255, 0, 0);      // rouge
      }
        nb_mesure_c = 0;
      }
      // writeSD(tempMem, humMem, now);
    }
  }

  alarm(isMuted, lastStatus, median_temp, ALARM_TEMP, nowMs);
}