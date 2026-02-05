#pragma once

#include <Arduino.h>

class ChainableLED;

void setAirQualityColor(ChainableLED& leds, int ledIndex, uint16_t eco2ppm);
void setTemperatureColor(ChainableLED& leds, int ledIndex, float tempC, float minC, float maxC);

