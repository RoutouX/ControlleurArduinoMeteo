// Leds.cpp
#include "Leds.h"

void startupAnimation(ChainableLED& leds) {
  // Étape 1 : alternance bleue
  for (int i = 0; i < 4; i++) {
    leds.setColorRGB(0, 0, 0, 255);
    leds.setColorRGB(1, 0, 0, 0);
    delay(150);

    leds.setColorRGB(0, 0, 0, 0);
    leds.setColorRGB(1, 0, 0, 255);
    delay(150);
  }

  // Étape 2 : flash blanc
  for (int i = 0; i < 2; i++) {
    leds.setColorRGB(0, 255, 255, 255);
    leds.setColorRGB(1, 255, 255, 255);
    delay(80);

    leds.setColorRGB(0, 0, 0, 0);
    leds.setColorRGB(1, 0, 0, 0);
    delay(80);
  }

  // Étape 3 : validation vert
  leds.setColorRGB(0, 0, 255, 0);
  leds.setColorRGB(1, 0, 255, 0);
}
