#include "soil_moisture.h"

#include "pins.h"

#include <Arduino.h> 

#define TOP_MOISTURE_SENSOR_RESISTANCE 4095

int measurements[NUMBER_OF_SOIL_MOISTURE_SENSORS];

void setupSoilMoisturePins() {
    pinMode(MULTIPLEXER_ADDRESS_A_PIN, OUTPUT);
    pinMode(MULTIPLEXER_ADDRESS_B_PIN, OUTPUT);
    pinMode(MULTIPLEXER_ADDRESS_C_PIN, OUTPUT);
}

void setAddress(int address) {
  digitalWrite(MULTIPLEXER_ADDRESS_A_PIN, (bool)(address & 1));
  digitalWrite(MULTIPLEXER_ADDRESS_B_PIN, (bool)(address & 2));
  digitalWrite(MULTIPLEXER_ADDRESS_C_PIN, (bool)(address & 4));
}

int measureMoistureInPercentages(int address) {
    setAddress(address);

    int analogValue = analogRead(SOIL_MOISTURE_ANALOG_READ_PIN);
    return (TOP_MOISTURE_SENSOR_RESISTANCE - analogValue) * 100 / TOP_MOISTURE_SENSOR_RESISTANCE;
}

const int * getMoistureMeasurements() {
    for (int address = 0; address < NUMBER_OF_SOIL_MOISTURE_SENSORS; address++) {
        measurements[address] = measureMoistureInPercentages(address);
    }

    return measurements;
}


