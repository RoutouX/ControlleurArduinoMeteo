#include <SPI.h>
#include <SD.h>
#include "Music.h"
#include "Math.h"
#include "config.h"
#include "Leds.h"
#include "PrintSD.h"
#include <ArduinoBLE.h>
#include "BLE.h"
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
//                         BLE
// =======================================================

BleManager ble;

static void onBleConnected() {
  Serial.println("[APP] BLE connected callback");
}

static void onBleDisconnected() {
  Serial.println("[APP] BLE disconnected callback");
}

static void onBleData(const uint8_t* data, size_t len) {
  static char line[96];
  size_t n = (len < sizeof(line)-1) ? len : sizeof(line)-1;
  memcpy(line, data, n);
  line[n] = '\0';

  Serial.print("[APP] PAYLOAD: ");
  Serial.println(line);

  // parse seq (avant le premier ';')
  char* semi = strchr(line, ';');
  if (!semi) return;
  *semi = '\0';

  uint32_t seq = (uint32_t)strtoul(line, nullptr, 10);

  bool ok = ble.writeAckU32(seq);
  Serial.print("[APP] ACK ");
  Serial.print(seq);
  Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FAIL");
}


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

  //Led
  leds.setColorRGB(0, 255, 255, 255);
  leds.setColorRGB(1, 255, 255, 255);

  BleManager::Config cfg;
  cfg.targetName  = "StationAir";
  cfg.serviceUuid = "181A";
  cfg.payloadUuid = "3006";
  cfg.ackUuid     = "3007";
  cfg.reconnectIntervalMs = 60000;

  ble.onConnected(onBleConnected);
  ble.onDisconnected(onBleDisconnected);
  ble.onData(onBleData);

  if (!ble.begin(cfg)) {
    Serial.println("[APP] BLE init failed");
    leds.setColorRGB(0, 255, 0, 0);
    leds.setColorRGB(1, 255, 0, 0);
    clean_exit();
  }

  startupAnimation(leds);
  leds.setColorRGB(0, 255, 255, 255);
  leds.setColorRGB(1, 255, 255, 255);
  Serial.println("Station Meteo prete");

}

// =======================================================
//                          LOOP
// =======================================================
void loop() {
  unsigned long nowMs = millis();
  ble.tick();
  // ------------------ Button debounce (your code) ------------------
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

  // ------------------ Sensor + median + LED (your code) ------------------
  if (nowMs - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = nowMs;
    noInterrupts();
    int status = am2302.read();
    interrupts();
    lastStatus = status;
    
    if (status == 0) {
      DateTime now = rtc.now();
      float temp = am2302.get_Temperature();
      float humi  = am2302.get_Humidity();
      nb_mesure_temp_t[nb_mesure_c] = temp;
      nb_mesure_humi_t[nb_mesure_c] = humi;
      nb_mesure_c++;

      if (nb_mesure_c == 10) {
        median_temp = mediane(nb_mesure_temp_t, 10);
        median_humi = mediane(nb_mesure_humi_t, 10);
        Serial.print(" -> Temp: "); Serial.print(median_temp);
        Serial.print(" C | Hum: "); Serial.print(median_humi);
        Serial.println(" %");

        // LED
        if (median_temp < 22.0) {
          leds.setColorRGB(0, 0, 0, 255);
        } else if (median_temp <= ALARM_TEMP) {
          leds.setColorRGB(0, 0, 255, 0);
        } else {
          leds.setColorRGB(0, 255, 0, 0);
        }

        nb_mesure_c = 0;
      }

      // writeSD(tempMem, humMem, now);
    } else {
      Serial.println("Sensor status != 0");
      Serial.print("Sensor status = ");
      Serial.println(status);
    }
  }

  alarm(isMuted, lastStatus, median_temp, ALARM_TEMP, nowMs);
}

void clean_exit() {
  BLE.end();
  SD.end();
  exit(1);
}