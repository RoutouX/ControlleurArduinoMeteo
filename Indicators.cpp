#include "Indicators.h"

#include <ChainableLED.h>

void setAirQualityColor(ChainableLED& leds, int ledIndex, uint16_t eco2ppm) {
  int r = 0, g = 0, b = 0;
  if (eco2ppm < 400) eco2ppm = 400;
  if (eco2ppm > 1000) eco2ppm = 1000;
  float ratio = (eco2ppm - 400.0f) / (1000.0f - 400.0f);
  r = (int)(ratio * 255.0f);
  g = (int)((1.0f - ratio) * 255.0f);
  leds.setColorRGB(ledIndex, r, g, b);
}

void setTemperatureColor(ChainableLED& leds, int ledIndex, float tempC, float minC, float maxC) {
  if (isnan(tempC)) return;

  if (tempC < minC) {
    leds.setColorRGB(ledIndex, 0, 0, 255);      // blue
  } else if (tempC <= maxC) {
    leds.setColorRGB(ledIndex, 0, 255, 0);      // green
  } else {
    leds.setColorRGB(ledIndex, 255, 0, 0);      // red
  }
}

