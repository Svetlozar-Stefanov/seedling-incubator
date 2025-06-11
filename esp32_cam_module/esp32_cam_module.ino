#include <WiFi.h>
#include <WiFiUdp.h>
#include <Arduino.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

#include "pins.h"
#include "server.h"
#include "camera.h"
#include "config_server.h"
#include "actuator_master.h"

const char* default_ssid = "GalaxyA71A333";
const char* default_password = "12341234";
const char* default_host_store = "https://seedling-incubator-default-rtdb.europe-west1.firebasedatabase.app/ip.json";

String ssid = default_ssid;
String password = default_password;
String host_store = default_host_store;

void setupSystem() {
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);  // Fully reset Wi-Fi subsystem
  delay(500);

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

  Preferences preferences;
  preferences.begin("wifi-config", true);
  host_store = preferences.getString("host_store", default_host_store);
  preferences.end();

  String hub_ip = "";
  // Get hub ip
  HTTPClient http_store;
  http_store.begin(host_store);
  int httpResponseCode = http_store.GET();
  if (httpResponseCode == 200) {
    String payload = http_store.getString();

    StaticJsonDocument<200> doc;
    DeserializationError error = deserializeJson(doc, payload);

    if (error) {
      Serial.print("JSON parse error: ");
      Serial.println(error.c_str());
      http_store.end();
      return;
    }

    // Read "ip" value
    const char* ipValue = doc["ip"];
    hub_ip = String(ipValue);
    http_store.end();
  }
  else {
    Serial.print("HTTP request for hub ip failed. Code: ");
    Serial.println(httpResponseCode);
    http_store.end();
    return;
  }

  // Send deivce data to hub
  // Get device data
  String mac = WiFi.macAddress();
  IPAddress ip = WiFi.localIP();

  // Create body
  String body = "{";
  body += "\t\"mac\": \"" + mac + "\",\n";
  body += "\t\"ip\": \"" + ip.toString() + "\"\n";
  body += "}";

  String registration_url = "http://" + hub_ip + ":8080/api/esp/register";

  HTTPClient http;
  http.begin(registration_url);
  http.addHeader("Content-Type", "application/json");

  httpResponseCode = http.POST(body);
  if (httpResponseCode == 200) {
    Serial.println("Device info sent to hub at: " + hub_ip);
    http.end();
    return;
  }
  Serial.println("Could not register with hub, consider rebooting.");
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
    return;
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
