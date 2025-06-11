#include "pins.h"

#include <Arduino.h>

int getLightLevel() {
  return analogRead(LIGHT_PIN);
}
