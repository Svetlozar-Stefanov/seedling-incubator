#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Arduino.h>

WebServer config_server(80);
Preferences preferences;

void handle_root() {
  config_server.send(200, "text/html", R"rawliteral(
    <form action="/save" method="POST">
      SSID: <input type="text" name="ssid"><br>
      Password: <input type="password" name="password"><br>
      Host IP: <input type="text" name="hostip"><br>
      <input type="submit" value="Save">
    </form>
  )rawliteral");
}

void handle_save() {
  String ssid = config_server.arg("ssid");
  String password = config_server.arg("password");
  String hostip = config_server.arg("hostip");

  preferences.begin("wifi-config", false);
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("hostip", hostip);
  preferences.end();

  config_server.send(200, "text/html", "Saved! Rebooting in 10 seconds, remove config pin...");
  delay(10000);
  ESP.restart();
}

void start_config_server() {
  WiFi.softAP("ESP32_Config");

  config_server.on("/", handle_root);
  config_server.on("/save", HTTP_POST, handle_save);
  config_server.begin();
  Serial.println("Config portal started!");
  Serial.print("Config portal started! Use 'http://");
  Serial.print(WiFi.localIP());
  Serial.println("/' to set device config");

  while (true) {
    config_server.handleClient();
  }
}