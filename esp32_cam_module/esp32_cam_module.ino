#include <WiFi.h>
#include <Preferences.h>
#include <Arduino.h>

#include "pins.h"
#include "server.h"
#include "camera.h"
#include "config_server.h"
#include "actuator_master.h"

String ssid = "GalaxyA71A333";
String password = "12341234";
String hub_ip = "192.168.27.213";

void setupSystem() {
  WiFi.mode(WIFI_AP_STA); 
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  Serial.print("Wi-Fi Channel: ");
  Serial.println(WiFi.channel());

  server_err_t serverErr = startServer();
  if(!serverErr) {
    Serial.print("Server Live! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");

    camera_err_t cameraErr = setupCamera();
    if(!cameraErr) {
      Serial.print("Camera Live! Use 'http://");
      Serial.print(WiFi.localIP());
      Serial.println("/capture' to request a capture");
    } else {
      Serial.printf("ERROR: Could not start camera. %s\n",
        getCameraErrorMessage(cameraErr));
    }
  }

  printMacAddress();
  actuator_err_t actuatorErr = connectToActuatorSlave();
  if (!actuatorErr) {
    Serial.print("Actuator Slave Live! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("/actuator/{cmd}' to request an action");
  }
  else {
    Serial.printf("ERROR: Could not connect to ESP32 slave. %s\n",
      getActuatorErrorMessage(actuatorErr));
  }
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  pinMode(CONFIG_PIN, INPUT_PULLUP);
  delay(100);
  
  // If config pin is shorted enter config mode
  if (digitalRead(CONFIG_PIN) == LOW) {
    Serial.println("Reset pin pressed. Starting config mode...");
    start_config_server();
  }
  
  Preferences preferences;
  preferences.begin("wifi-config", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  hub_ip = preferences.getString("hub_ip", "");
  preferences.end();

  if (ssid != "") {
    setupSystem();
  } else {
    Serial.println("No configuration available, short the config pin to enter config mode.");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  
}
