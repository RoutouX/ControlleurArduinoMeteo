#pragma once
#include <Arduino.h>
#include "RTClib.h"

bool writeSD(float tempC, float humPct, const DateTime& time);
