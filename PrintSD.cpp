#include "PrintSD.h"

#include <SD.h>

#include "Log.h"

static void print2(Print& p, int v) {
  if (v < 10) p.print('0');
  p.print(v);
}

bool writeSD(float tempC, float humPct, const DateTime& time) {
  char fileName[13];
  snprintf(fileName, sizeof(fileName), "%02d%02d%04d.csv", time.day(), time.month(), time.year());

  File dataFile = SD.open(fileName, FILE_WRITE);
  if (!dataFile) {
    logInfo("SD", String("Open failed ") + fileName);
    return false;
  }

  print2(dataFile, time.hour()); dataFile.print(":");
  print2(dataFile, time.minute()); dataFile.print(":");
  print2(dataFile, time.second());
  dataFile.print(";"); dataFile.print(tempC);
  dataFile.print(";"); dataFile.println(humPct);
  dataFile.close();
  return true;
}
