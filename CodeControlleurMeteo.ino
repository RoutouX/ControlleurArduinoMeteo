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
#include <WiFiS3.h> 

// --- Config Clim / WiFi ---
const char* WIFI_SSID = "A4_IOT_CESI";
const char* WIFI_PASS = "WrVqBaGJRbRVh857KQzEmRtcnToVvo8kBh7VhDd8MjRUmHpEYS";
const char* CLIM_HOST = "192.168.100.162";
const int   CLIM_PORT = 8080;
const float TEMP_MIN = 21.0;   // seuil bas accepté
const float TEMP_MAX = 24.0;   // seuil haut accepté
const float TEMP_HYST = 0.4;   // hystérésis anti yoyo
const char* DASHBOARD_HOST = "192.168.100.10"; 
const int   DASHBOARD_PORT = 3000;              

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
bool climOn = false;

// =======================================================
//                         BLE
// =======================================================

uint32_t g_seq = 0;
uint32_t g_timestamp = 0;
char g_datetime[24] = {0};
int eco2_value = 0;
bool g_payloadValid = false;


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

  // Découpe par ';'
  char* token;
  char* rest = line;

  // ---- seq ----
  token = strtok_r(rest, ";", &rest);
  if (!token) return;
  g_seq = strtoul(token, nullptr, 10);

  // ---- timestamp ----
  token = strtok_r(nullptr, ";", &rest);
  if (!token) return;
  g_timestamp = strtoul(token, nullptr, 10);

  // ---- date/time ----
  token = strtok_r(nullptr, ";", &rest);
  if (!token) return;
  strncpy(g_datetime, token, sizeof(g_datetime) - 1);

  // ---- value ----
  token = strtok_r(nullptr, ";", &rest);
  if (!token) return;
  eco2_value = atoi(token);

  g_payloadValid = true;

  Serial.print("  eco2 = "); Serial.println(eco2_value);

  // ACK BLE
  bool ok = ble.writeAckU32(g_seq);
  Serial.print("[APP] ACK ");
  Serial.print(g_seq);
  Serial.print(" -> ");
  Serial.println(ok ? "OK" : "FAIL");

  setAirQualityColor(eco2_value);
}

void setAirQualityColor(uint16_t eco2ppm) {
  int r = 0, g = 0, b = 0;
  if (eco2ppm < 400) eco2ppm = 400;
  if (eco2ppm > 1000) eco2ppm = 1000;
  float ratio = (eco2ppm - 400.0) / (1000.0 - 400.0);
  r = ratio * 255;
  g = (1.0 - ratio) * 255;
  leds.setColorRGB(1, r, g, 0);
}

const char* wifiStatusToStr(int s) {
  switch (s) {
    case WL_CONNECTED: return "CONNECTED";
    case WL_IDLE_STATUS: return "IDLE";
    case WL_NO_SSID_AVAIL: return "NO_SSID";
    case WL_SCAN_COMPLETED: return "SCAN_DONE";
    case WL_CONNECT_FAILED: return "CONNECT_FAIL";
    case WL_CONNECTION_LOST: return "CONN_LOST";
    case WL_DISCONNECTED: return "DISCONNECTED";
    case WL_NO_MODULE: return "NO_MODULE";
    default: return "UNKNOWN";
  }
}

bool ensureWifiConnected() {
  static unsigned long lastAttemptMs = 0;
  const unsigned long WIFI_RETRY_MS = 5000;

  if (WiFi.status() == WL_CONNECTED) return true;

  // Evite de spammer les tentatives
  if (millis() - lastAttemptMs < WIFI_RETRY_MS) return false;
  lastAttemptMs = millis();

  if (strcmp(WIFI_SSID, "YOUR_WIFI") == 0) {
    Serial.println("[CLIM] WiFi non configuré (SSID/PASS)");
    return false;
  }

  Serial.print("[CLIM] WiFi begin SSID=");
  Serial.println(WIFI_SSID);

  // Force static IP configuration before connecting
  WiFi.config(localIP, dns, gateway, subnet);

  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    int st = WiFi.status();
    Serial.print("[CLIM] ... ");
    Serial.println(wifiStatusToStr(st));
    delay(250);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[CLIM] WiFi OK, IP=");
    Serial.println(WiFi.localIP());
    return true;
  }

  Serial.print("[CLIM] WiFi KO status=");
  Serial.println(wifiStatusToStr(WiFi.status()));
  Serial.print("[CLIM] Firmware=");
  Serial.println(WiFi.firmwareVersion());
  return false;
}

void sendClimCommand(const char* cmd, const char* value = nullptr) {
  if (!ensureWifiConnected()) {
    Serial.println("[CLIM] WiFi KO");
    return;
  }
  WiFiClient client;
  if (!client.connect(CLIM_HOST, CLIM_PORT)) {
    Serial.println("[CLIM] Connexion clim échouée");
    return;
  }
  String url = "/clim?command=";
  url += cmd;
  if (value) {
    url += "&value=";
    url += value;
  }
  client.print(String("GET ") + url + " HTTP/1.1\r\nHost: " + CLIM_HOST + "\r\nConnection: close\r\n\r\n");
  client.stop();
}

void regulateClim(float roomTemp) {
  if (isnan(roomTemp)) return;

  if (roomTemp > TEMP_MAX + TEMP_HYST) {
    char target[8];
    dtostrf(TEMP_MAX, 0, 1, target);
    sendClimCommand("TEMP", target);
    if (!climOn) sendClimCommand("ON");
    climOn = true;
  } else if (roomTemp < TEMP_MIN - TEMP_HYST) {
    char target[8];
    dtostrf(TEMP_MIN, 0, 1, target);
    sendClimCommand("TEMP", target);
    if (!climOn) sendClimCommand("ON");
    climOn = true;
  } else if (climOn && roomTemp > TEMP_MIN + TEMP_HYST && roomTemp < TEMP_MAX - TEMP_HYST) {
    sendClimCommand("OFF");
    climOn = false;
  }
}

void sendToDashboard(float temp, float hum, int co2, bool climState) {
  if (!ensureWifiConnected()) return;

  WiFiClient client;
  if (!client.connect(DASHBOARD_HOST, DASHBOARD_PORT)) {
    Serial.println("[DASH] Erreur connexion serveur");
    return;
  }

  // 1. Préparation du Timestamp (depuis le RTC)
  DateTime now = rtc.now();
  char timeBuffer[25];
  // Format ISO 8601 classique : YYYY-MM-DDTHH:MM:SS
  sprintf(timeBuffer, "%04d-%02d-%02dT%02d:%02d:%02d", 
          now.year(), now.month(), now.day(), 
          now.hour(), now.minute(), now.second());

  // 2. Construction du JSON manuel
  // Format demandé : [{ "idData": 0, "timestamp": "...", "valueCO2": ... }]
  String json = "[{";
  json += "\"idData\": 0,"; 
  json += "\"timestamp\": \"" + String(timeBuffer) + "\",";
  json += "\"valueCO2\": " + String(co2) + ",";
  json += "\"valueTemp\": " + String(temp) + ",";
  json += "\"valueHum\": " + String(hum) + ",";
  // Attention : climStatus attend "true"/"false" (String) ou true/false (Booléen) ? 
  // Dans le doute standard JSON, je mets un booléen, sinon ajoute des guillemets \" autour
  json += "\"climStatus\": " + String(climState ? "true" : "false");
  json += "}]";

  // 3. Envoi de la requête HTTP PUT
  client.println("PUT /arduino/publish HTTP/1.1");
  client.println("Host: " + String(DASHBOARD_HOST));
  client.println("Content-Type: application/json");
  client.print("Content-Length: ");
  client.println(json.length());
  client.println("Connection: close");
  client.println(); // Ligne vide obligatoire entre headers et body
  client.println(json);

  Serial.println("[DASH] Données envoyées !");
  
  // Petit délai pour laisser le temps au serveur de répondre (optionnel)
  delay(50);
  client.stop();
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

  ensureWifiConnected();
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

        // --- Gestion des LEDs existante ---
        if (median_temp < 22.0) {
          leds.setColorRGB(0, 0, 0, 255);
        } else if (median_temp <= 24.0) { // J'ai remplacé ALARM_TEMP par une valeur car il n'était pas défini dans ton snippet
           leds.setColorRGB(0, 0, 255, 0);
        } else {
          leds.setColorRGB(0, 255, 0, 0);
        }
        
        // --- Régulation Clim existante ---
        regulateClim(median_temp);
        
        // ============================================
        // AJOUT ICI : ENVOI AU SERVEUR
        // ============================================
        // On envoie : Température, Humidité, CO2 (BLE), et État Clim
        sendToDashboard(median_temp, median_humi, eco2_value, climOn);

        nb_mesure_c = 0;
      }
      // writeSD(tempMem, humMem, now);  // disabled: tempMem/humMem not defined
    } else {
      Serial.println("Sensor status != 0");
      Serial.print("Sensor status = ");
      Serial.println(status);
    }
  }
  alarm(isMuted, eco2_value, co2_buzz, nowMs);
}

void clean_exit() {
  BLE.end();
  SD.end();
  exit(1);
}