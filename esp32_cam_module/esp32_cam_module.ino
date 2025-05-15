#include <WiFi.h>
#include "esp_http_server.h"
#include "esp_camera.h"

#include "camera.h"

// ===========================
// Enter your WiFi credentials
// ===========================
const char *ssid = "GalaxyA71A333";
const char *password = "12341234";

httpd_handle_t server = NULL;

esp_err_t capture_handler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.println("Camera capture failed");
    httpd_resp_send_500(req);
    return ESP_FAIL;
  }

  // Set headers and send image
  httpd_resp_set_type(req, "image/jpeg");
  httpd_resp_set_hdr(req, "Content-Disposition", "inline; filename=capture.jpg");
  esp_err_t res = httpd_resp_send(req, (const char *)fb->buf, fb->len);

  esp_camera_fb_return(fb);
  return res;
}

httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = capture_handler,
    .user_ctx = NULL
};

int startServer() {
  httpd_config_t config = HTTPD_DEFAULT_CONFIG();
  config.server_port = 80;

  if (httpd_start(&server, &config) == ESP_OK) {
    httpd_register_uri_handler(server, &capture_uri);
    Serial.println("Web server started");
    return 0;
  } else {
    Serial.println("Failed to start web server");
    return 1;
  }
}

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

  int serverStarted = !startServer();
  if(serverStarted) {
    Serial.print("Server Live! Use 'http://");
    Serial.print(WiFi.localIP());
    Serial.println("' to connect");

    int cameraStarted = !setupCamera();
    if(cameraStarted) {
      Serial.print("Camera Live! Use 'http://");
      Serial.print(WiFi.localIP());
      Serial.println("/capture' to request a capture");
    } else {
      Serial.println("Could not start camera.");
    }
  } else {
    Serial.println("Could not start server.");
  }
}

void loop() {
  // put your main code here, to run repeatedly:

}
