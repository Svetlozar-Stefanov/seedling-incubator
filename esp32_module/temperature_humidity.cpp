#include "pins.h"

#include <Wire.h>
#include <Adafruit_AHTX0.h>

Adafruit_AHTX0 sensor;

void setupI2CPins() {
  Wire.begin(SDA_PIN, SCL_PIN);

  if (!sensor.begin()) {
    Serial.println("Failed to find AHT10 sensor. Check wiring!");
    while (1) delay(10);
  }
  Serial.println("AHT10 found!");
}

float getTemperature() {
  sensors_event_t humidity, temp;
  sensor.getEvent(&humidity, &temp);

  return temp.temperature;
}

float getAirHumidity() {
  sensors_event_t humidity, temp;
  sensor.getEvent(&humidity, &temp);

  return humidity.relative_humidity;
}