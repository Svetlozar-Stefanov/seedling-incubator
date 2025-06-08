#include <WiFi.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <Preferences.h>

#include "pins.h"
#include "server.h"
#include "camera.h"
#include "config_server.h"
#include "actuator_master.h"

const char* default_ssid = "GalaxyA71A333";
const char* default_password = "12341234";
const char* default_hub_ip = "192.168.27.213";

String ssid = default_ssid;
String password = default_password;
String hub_ip = default_hub_ip;

void setupSystem() {
  // Connect to hub hotspot
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

  // Start subsystems
  camera_err_t cameraErr = setupCamera();
  if(!cameraErr) {
    Serial.print("Camera Live! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("/capture' to request a capture");
  } else {
    Serial.printf("ERROR: Could not start camera. %s\n",
      getCameraErrorMessage(cameraErr));
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

  server_err_t serverErr = startServer();
  if(!serverErr) {
    Serial.print("Server Live! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");
  } else {
    Serial.printf("ERROR: Could not start device server. %s\n",
      getServerErrorMessage(serverErr));
  }

  // Connect device to hub after subsystems setup
  Preferences preferences;
  preferences.begin("wifi-config", true);
  hub_ip = preferences.getString("hub_ip", default_hub_ip);
  preferences.end();

  // Get device data
  String mac = WiFi.macAddress();
  String ip = WiFi.localIP().toString();

  // Setup registration url
  String registration_url = "http://" + hub_ip + ":8000/devices/register";

  // Create body
  String body = "{";
  body += "\"mac\": \"" + mac + "\",";
  body += "\"ip\": \"" + ip + "\"";
  body += "}";

  HTTPClient http;
  http.begin(registration_url);
  http.addHeader("Content-Type", "application/json");

  int httpResponseCode = http.POST(body);
  if (httpResponseCode > 0) {
    Serial.printf("POST response code: %d\n", httpResponseCode);
    if (httpResponseCode != 200) {
      String response = http.getString();
      Serial.println("Server response: " + response);
    }
  } else {
    Serial.printf("POST failed: %s\n", http.errorToString(httpResponseCode).c_str());
  }

  http.end();
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
  ssid = preferences.getString("ssid", default_ssid);
  password = preferences.getString("password", default_password);
  preferences.end();

  setupSystem();
}

void loop() {
  // put your main code here, to run repeatedly:
  
}
