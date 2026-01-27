#pragma once
#include <Arduino.h>
#include <SD.h>
#include "RTClib.h"

static void print2(Print& p, int v) {
  if (v < 10) p.print('0');
  p.print(v);
}
void writeSD(float temp, float humi, DateTime time){
      char fileName[13];
      sprintf(fileName, "%02d%02d%04d.csv", time.day(), time.month(), time.year());

      File dataFile = SD.open(fileName, FILE_WRITE);
      if (dataFile) {
        print2(dataFile, time.hour()); dataFile.print(":");
        print2(dataFile, time.minute()); dataFile.print(":");
        print2(dataFile, time.second());
        dataFile.print(";"); dataFile.print(temp);
        dataFile.print(";"); dataFile.println(humi);
        dataFile.close();
      } else {
        Serial.print("ERREUR : impossible d'ouvrir ");
        Serial.println(fileName);
      }
}