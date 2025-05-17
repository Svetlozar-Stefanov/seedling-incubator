#include <WiFi.h>

#include "server.h"
#include "camera.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "GalaxyA71A333";
const char *password = "12341234";

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  WiFi.setSleep(false);

  Serial.print("WiFi connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

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
  } else {
    Serial.printf("ERROR: Could not start server. %s\n", 
      getServerErrorMessage(serverErr));
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
