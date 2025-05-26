#include <cstring>
#include "server.h"

#include "actuator_master.h"

#include "esp_camera.h"
#include "esp_http_server.h"

#include <Arduino.h>

httpd_handle_t server = NULL;

esp_err_t captureHandler(httpd_req_t *req) {
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) {
    Serial.printf("Camera capture failed\n");
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

esp_err_t readMoistureHandler(httpd_req_t *req) {
  sendCommand(READ_SOIL_MOISTURE);

  unsigned long start = millis();
  while (!isResponseReceived() && millis() - start < 5000);
  if (millis() - start >= 5000) {
    return ESP_ERR_TIMEOUT;
  }

  httpd_resp_set_type(req, "application/json");
  response res = getResponse();
  char json[1024];
  int offset = 0;

  offset += snprintf(json + offset, sizeof(json) - offset, "[");

  for (int i = 0; i < res.len; i++) {
      offset += snprintf(json + offset, sizeof(json) - offset,
                         "{\"sensor_id\":%d,\"value\":%d}%s",
                         i,
                         res.data[i],
                         (i < res.len - 1) ? "," : "");
  }
  
  offset += snprintf(json + offset, sizeof(json) - offset, "]");

  esp_err_t err = httpd_resp_send(req, json, strlen(json));

  return err;
}

// Uri definitions
httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = captureHandler,
    .user_ctx = NULL
};

httpd_uri_t moisture_uri = {
    .uri = "/actuator/moisture",
    .method = HTTP_GET,
    .handler = readMoistureHandler,
    .user_ctx = NULL
};

server_err_t startServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
  
    if (httpd_start(&server, &config) == ESP_OK) {
      httpd_register_uri_handler(server, &capture_uri);
      httpd_register_uri_handler(server, &moisture_uri);
      return SERVER_SUCCESS;
    } else {
      return SERVER_ERR_INITIALIZATION_FAILED;
    }
}

const char* getServerErrorMessage(server_err_t err) {
    switch (err) {
        case SERVER_SUCCESS: return "Success";
        case SERVER_ERR_INITIALIZATION_FAILED: return "Initialization failed.";
        default: return "Unknown error";
    }
}