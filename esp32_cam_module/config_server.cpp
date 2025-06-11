#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Arduino.h>

#include "esp_wifi.h"

WebServer config_server(80);
Preferences preferences;

void handle_root() {
  config_server.send(200, "text/html", R"rawliteral(
    <form action="/save" method="POST">
      SSID: <input type="text" name="ssid"><br>
      Password: <input type="password" name="password"><br>
      Host IP Storage Url: <input type="text" name="host_storage"><br>
      <input type="submit" value="Save">
    </form>
  )rawliteral");
}

void handle_save() {
  String ssid = config_server.arg("ssid");
  String password = config_server.arg("password");
  String host_storage = config_server.arg("host_storage");

  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("host_storage", host_storage);
  preferences.end();

  config_server.send(200, "text/html", "Saved! Rebooting in 10 seconds, remove config pin...");
  delay(10000);
  
  ESP.restart();
}

void start_config_server() {
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);  // Fully reset Wi-Fi subsystem
  delay(500);

  WiFi.softAP("ESP32");
  String mac = WiFi.macAddress();
  
  WiFi.disconnect(true);
  WiFi.softAPdisconnect(true);
  WiFi.mode(WIFI_OFF);  // Fully reset Wi-Fi subsystem
  delay(500);
  WiFi.softAP("ESP32_" + mac + "_Config");

  config_server.on("/", handle_root);
  config_server.on("/save", HTTP_POST, handle_save);
  config_server.begin();
  Serial.println("Config portal started!");

  while (true) {
    config_server.handleClient();
  }
}