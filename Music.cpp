// Music.cpp
#include "Music.h"
#include "Config.h"

int tempo = 120;

const int melody[] = {
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

// ---- Global instance ----
MusicPlayer player;

// ---- Implementations ----
MusicPlayer::MusicPlayer()
  : active(false),
    index(0),
    phaseEndMs(0),
    inPause(false),
    wholenoteMs(0) {}

void MusicPlayer::begin(int bpm) {
  wholenoteMs = (60000UL * 4UL) / (unsigned long)bpm;
}

void MusicPlayer::start() {
  if (!active) {
    active = true;
    index = 0;
    inPause = false;
    phaseEndMs = 0;
  }
}

void MusicPlayer::stop() {
  active = false;
  inPause = false;
  noTone(pinBuzzer);
  digitalWrite(pinBlink, LOW);
}

int MusicPlayer::computeNoteDuration(int divider) {
  if (divider > 0) return wholenoteMs / divider;
  float base = (float)wholenoteMs / (float)abs(divider);
  return (int)(base * 1.5f);
}

void MusicPlayer::tick(unsigned long nowMs) {
  if (!active) return;

  if (phaseEndMs != 0 && nowMs < phaseEndMs) return;

  if (!inPause && phaseEndMs != 0) {
    noTone(pinBuzzer);
    digitalWrite(pinBlink, LOW);
    inPause = true;

    int divider = melody[index * 2 + 1];
    int noteDuration = computeNoteDuration(divider);

    int pauseMs = max(5, noteDuration - (int)(noteDuration * 0.90f));
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

  int playMs = max(10, (int)(noteDuration * 0.90f));

  if (pitch == REST) {
    noTone(pinBuzzer);
    digitalWrite(pinBlink, LOW);
  } else {
    tone(pinBuzzer, pitch);
    digitalWrite(pinBlink, HIGH);
  }

  phaseEndMs = nowMs + (unsigned long)playMs;
}

void alarm(bool isMuted, float eco2, float co2_buzz, unsigned long nowMs) {
  bool shouldAlarm = !isMuted
                     && !isnan(eco2)
                     && eco2 > co2_buzz;

  if (shouldAlarm) {
    player.start();
  } else {
    player.stop();
  }
  player.tick(nowMs);
}
