#include <SPI.h>
#include <SD.h>
#include "RTClib.h"
#include <AM2302-Sensor.h>
#include <ChainableLED.h>

// ----------------- HW PINS -----------------
const byte DHTPIN = 2;

const int pinBuzzer = 10;      // buzzer sur D6
const int pinSD = 4;          // CS SD sur D4
const int pinButton = 5;      // module bouton sur D5

#define NUM_LEDS 1
ChainableLED leds(7, 8, NUM_LEDS);

const int pinBlink = LED_BUILTIN;

// ----------------- DEVICES -----------------
AM2302::AM2302_Sensor am2302{ DHTPIN };
RTC_DS1307 rtc;

// ----------------- APP PARAMS -----------------
const unsigned long SENSOR_PERIOD_MS = 2000;
const float ALARM_TEMP = 24.8;
bool alarmEnabled = true;

unsigned long lastSensorMs = 0;

// --------- MUTE BUTTON (2 minutes) ----------
const unsigned long MUTE_MS = 120000UL; // 2 minutes
unsigned long muteUntilMs = 0;

// ----------------- HELPERS -----------------
static void print2(Print& p, int v) {
  if (v < 10) p.print('0');
  p.print(v);
}

// =======================================================
//                    PINK PANTHER NOTES
// =======================================================
#define NOTE_B0  31
#define NOTE_C1  33
#define NOTE_CS1 35
#define NOTE_D1  37
#define NOTE_DS1 39
#define NOTE_E1  41
#define NOTE_F1  44
#define NOTE_FS1 46
#define NOTE_G1  49
#define NOTE_GS1 52
#define NOTE_A1  55
#define NOTE_AS1 58
#define NOTE_B1  62
#define NOTE_C2  65
#define NOTE_CS2 69
#define NOTE_D2  73
#define NOTE_DS2 78
#define NOTE_E2  82
#define NOTE_F2  87
#define NOTE_FS2 93
#define NOTE_G2  98
#define NOTE_GS2 104
#define NOTE_A2  110
#define NOTE_AS2 117
#define NOTE_B2  123
#define NOTE_C3  131
#define NOTE_CS3 139
#define NOTE_D3  147
#define NOTE_DS3 156
#define NOTE_E3  165
#define NOTE_F3  175
#define NOTE_FS3 185
#define NOTE_G3  196
#define NOTE_GS3 208
#define NOTE_A3  220
#define NOTE_AS3 233
#define NOTE_B3  247
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define NOTE_C6  1047
#define NOTE_CS6 1109
#define NOTE_D6  1175
#define NOTE_DS6 1245
#define NOTE_E6  1319
#define NOTE_F6  1397
#define NOTE_FS6 1480
#define NOTE_G6  1568
#define NOTE_GS6 1661
#define NOTE_A6  1760
#define NOTE_AS6 1865
#define NOTE_B6  1976
#define NOTE_C7  2093
#define NOTE_CS7 2217
#define NOTE_D7  2349
#define NOTE_DS7 2489
#define NOTE_E7  2637
#define NOTE_F7  2794
#define NOTE_FS7 2960
#define NOTE_G7  3136
#define NOTE_GS7 3322
#define NOTE_A7  3520
#define NOTE_AS7 3729
#define NOTE_B7  3951
#define NOTE_C8  4186
#define NOTE_CS8 4435
#define NOTE_D8  4699
#define NOTE_DS8 4978
#define REST      0

int tempo = 120;

int melody[] = {
  REST,2, REST,4, REST,8, NOTE_DS4,8,
  NOTE_E4,-4, REST,8, NOTE_FS4,8, NOTE_G4,-4, REST,8, NOTE_DS4,8,
  NOTE_E4,-8, NOTE_FS4,8,  NOTE_G4,-8, NOTE_C5,8, NOTE_B4,-8, NOTE_E4,8, NOTE_G4,-8, NOTE_B4,8,
  NOTE_AS4,2, NOTE_A4,-16, NOTE_G4,-16, NOTE_E4,-16, NOTE_D4,-16,
  NOTE_E4,2, REST,4, REST,8, NOTE_DS4,4,

  NOTE_E4,-4, REST,8, NOTE_FS4,8, NOTE_G4,-4, REST,8, NOTE_DS4,8,
  NOTE_E4,-8, NOTE_FS4,8,  NOTE_G4,-8, NOTE_C5,8, NOTE_B4,-8, NOTE_G4,8, NOTE_B4,-8, NOTE_E5,8,
  NOTE_DS5,1,
  NOTE_D5,2, REST,4, REST,8, NOTE_DS4,8,
  NOTE_E4,-4, REST,8, NOTE_FS4,8, NOTE_G4,-4, REST,8, NOTE_DS4,8,
  NOTE_E4,-8, NOTE_FS4,8,  NOTE_G4,-8, NOTE_C5,8, NOTE_B4,-8, NOTE_E4,8, NOTE_G4,-8, NOTE_B4,8,

  NOTE_AS4,2, NOTE_A4,-16, NOTE_G4,-16, NOTE_E4,-16, NOTE_D4,-16,
  NOTE_E4,-4, REST,4,
  REST,4, NOTE_E5,-8, NOTE_D5,8, NOTE_B4,-8, NOTE_A4,8, NOTE_G4,-8, NOTE_E4,-8,
  NOTE_AS4,16, NOTE_A4,-8, NOTE_AS4,16, NOTE_A4,-8, NOTE_AS4,16, NOTE_A4,-8, NOTE_AS4,16, NOTE_A4,-8,
  NOTE_G4,-16, NOTE_E4,-16, NOTE_D4,-16, NOTE_E4,16, NOTE_E4,16, NOTE_E4,2,
};

const int songPairs = (sizeof(melody) / sizeof(melody[0])) / 2;

// =======================================================
//               NON-BLOCKING MUSIC PLAYER
// =======================================================
struct MusicPlayer {
  bool active = false;
  int index = 0;
  unsigned long phaseEndMs = 0;
  bool inPause = false;
  int wholenoteMs = 0;

  void begin(int bpm) {
    wholenoteMs = (60000 * 4) / bpm;
  }

  void start() {
    if (!active) {
      active = true;
      index = 0;
      inPause = false;
      phaseEndMs = 0;
    }
  }

  void stop() {
    active = false;
    inPause = false;
    noTone(pinBuzzer);
    digitalWrite(pinBlink, LOW);
  }

  int computeNoteDuration(int divider) {
    if (divider > 0) return wholenoteMs / divider;
    float base = (float)wholenoteMs / (float)abs(divider);
    return (int)(base * 1.5f);
  }

  void tick(unsigned long nowMs) {
    if (!active) return;

    if (phaseEndMs != 0 && nowMs < phaseEndMs) return;

    if (!inPause && phaseEndMs != 0) {
      noTone(pinBuzzer);
      digitalWrite(pinBlink, LOW);
      inPause = true;

      int divider = melody[index * 2 + 1];
      int noteDuration = computeNoteDuration(divider);

      int pauseMs = max(5, noteDuration - (int)(noteDuration * 0.90));
      phaseEndMs = nowMs + (unsigned long)pauseMs;
      return;
    }

    if (inPause) {
      inPause = false;
      index++;
      if (index >= songPairs) index = 0;
    }

    int pitch = melody[index * 2];
    int divider = melody[index * 2 + 1];
    int noteDuration = computeNoteDuration(divider);

    int playMs = max(10, (int)(noteDuration * 0.90));

    if (pitch == REST) {
      noTone(pinBuzzer);
      digitalWrite(pinBlink, LOW);
    } else {
      tone(pinBuzzer, pitch);
      digitalWrite(pinBlink, HIGH);
    }

    phaseEndMs = nowMs + (unsigned long)playMs;
  }
};

MusicPlayer player;

// =======================================================
//                     SENSOR STATE
// =======================================================
float tempMem = NAN;
float humMem = NAN;
uint8_t lastStatus = 255;

// =======================================================
//                         SETUP
// =======================================================
void setup() {
  Serial.begin(9600);
  while (!Serial) {}

  pinMode(pinBuzzer, OUTPUT);
  pinMode(pinBlink, OUTPUT);
  digitalWrite(pinBlink, LOW);

  // Module bouton: très souvent c'est une sortie "active LOW"
  // (si c'est l'inverse chez toi, je te dis plus bas quoi changer)
  pinMode(pinButton, INPUT_PULLUP);

  if (!rtc.begin()) {
    Serial.println("ERREUR : RTC introuvable !");
  }
  rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

  if (!SD.begin(pinSD)) {
    Serial.println("ERREUR : Carte SD non detectee !");
  }

  am2302.begin();
  delay(2000);
  Serial.println("Station Meteo prete");

  player.begin(tempo);
}

// =======================================================
//                          LOOP
// =======================================================
void loop() {
  unsigned long nowMs = millis();

  // -------------------------------------------------------
  // Bouton D5 (debounce) : par défaut on considère appui = LOW
  // -------------------------------------------------------
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

    // APPUI = LOW (active LOW)
    if (stableState == LOW) {
      muteUntilMs = nowMs + MUTE_MS;
      player.stop(); // coupe immédiatement la musique
      Serial.println("MUTE 2 minutes !");
    }
  }

  bool isMuted = (nowMs < muteUntilMs);

  // 1) Tâche capteur + SD + Serial + LED couleur (toutes les 2 secondes)
  if (nowMs - lastSensorMs >= SENSOR_PERIOD_MS) {
    lastSensorMs = nowMs;

    auto status = am2302.read();
    lastStatus = status;

    if (status == 0) {
      DateTime now = rtc.now();
      tempMem = am2302.get_Temperature();
      humMem  = am2302.get_Humidity();

      // ----- LOG SD -----
      char fileName[13];
      sprintf(fileName, "%02d%02d%04d.csv", now.day(), now.month(), now.year());

      File dataFile = SD.open(fileName, FILE_WRITE);
      if (dataFile) {
        print2(dataFile, now.hour()); dataFile.print(":");
        print2(dataFile, now.minute()); dataFile.print(":");
        print2(dataFile, now.second());
        dataFile.print(";"); dataFile.print(tempMem);
        dataFile.print(";"); dataFile.println(humMem);
        dataFile.close();
      } else {
        Serial.print("ERREUR : impossible d'ouvrir ");
        Serial.println(fileName);
      }

      // ----- SERIAL -----
      print2(Serial, now.hour()); Serial.print(":");
      print2(Serial, now.minute());
      Serial.print(" -> Temp: "); Serial.print(tempMem);
      Serial.print(" C | Hum: "); Serial.print(humMem);
      Serial.println(" %");

      // ----- LED couleur -----
      if (tempMem < 22.0) {
        leds.setColorRGB(0, 0, 0, 255);      // bleu
      } else if (tempMem <= ALARM_TEMP) {
        leds.setColorRGB(0, 0, 255, 0);      // vert
      } else {
        leds.setColorRGB(0, 255, 0, 0);      // rouge
      }

    } else {
      leds.setColorRGB(0, 100, 100, 0);
      Serial.print("Erreur lecture capteur. Code : ");
      Serial.println(status);
    }
  }

  // 2) Alarme musique (non-bloquante)
  bool shouldAlarm = alarmEnabled
                     && !isMuted
                     && lastStatus == 0
                     && !isnan(tempMem)
                     && tempMem > ALARM_TEMP;

  if (shouldAlarm) {
    player.start();
  } else {
    player.stop();
  }

  player.tick(nowMs);
}
