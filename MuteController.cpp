#include "MuteController.h"

#include "Log.h"
#include "Music.h"

MuteController::MuteController(int pinButton, unsigned long muteMs)
  : _pinButton(pinButton), _muteMs(muteMs) {}

void MuteController::begin() {
  pinMode(_pinButton, INPUT_PULLUP);
}

void MuteController::tick(unsigned long nowMs, MusicPlayer& player) {
  const unsigned long kDebounceMs = 30;

  bool readNow = digitalRead(_pinButton);
  if (readNow != _lastRead) {
    _lastRead = readNow;
    _lastChangeMs = nowMs;
  }

  if ((nowMs - _lastChangeMs) > kDebounceMs && readNow != _stableState) {
    _stableState = readNow;
    if (_stableState == LOW) {
      _muteUntilMs = nowMs + _muteMs;
      player.stop();
      logInfo("APP", "Muted");
    }
  }
}
