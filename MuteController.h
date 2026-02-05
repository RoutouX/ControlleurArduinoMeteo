#pragma once

#include <Arduino.h>

struct MusicPlayer;

class MuteController {
public:
  MuteController(int pinButton, unsigned long muteMs);

  void begin();
  void tick(unsigned long nowMs, MusicPlayer& player);

  bool isMuted(unsigned long nowMs) const { return nowMs < _muteUntilMs; }

private:
  const int _pinButton;
  const unsigned long _muteMs;

  bool _stableState = HIGH;
  bool _lastRead = HIGH;
  unsigned long _lastChangeMs = 0;
  unsigned long _muteUntilMs = 0;
};

