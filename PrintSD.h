#pragma once
#include <Arduino.h>
#include <SD.h>
#include "RTClib.h"

static void print2(Print& p, int v);
void writeSD(float temp, float humi, DateTime time);