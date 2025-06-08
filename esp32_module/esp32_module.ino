#include "actuator_slave.h"

#include "soil_moisture.h"
#include "temperature_humidity.h"

#include <WiFi.h>

#include "esp_wifi.h"

void setup() {
  Serial.begin(115200); // Start serial communication

  setupSoilMoisturePins();
  setupI2CPins();

  WiFi.mode(WIFI_STA);
  WiFi.STA.begin();
  WiFi.setSleep(false);

  esp_wifi_set_promiscuous(true);
  esp_wifi_set_channel(11, WIFI_SECOND_CHAN_NONE);
  esp_wifi_set_promiscuous(false);
  
  printMacAddress();

  actuator_err_t actuatorErr = setupActuatorSlave();
  if (actuatorErr) {
   Serial.printf("ERROR: Could not setup ESP32 slave. %s\n",
      getActuatorErrorMessage(actuatorErr));
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  
}