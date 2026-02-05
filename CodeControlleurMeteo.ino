#include "App.h"

static App& app() {
  static App instance;
  return instance;
}

void setup() {
  app().begin();
}

void loop() {
  app().tick();
}