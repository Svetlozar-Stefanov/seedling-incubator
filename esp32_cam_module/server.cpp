#include <cstring>
#include "server.h"

#include "actuator_master.h"

#include "esp_camera.h"
#include "img_converters.h"
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

esp_err_t index_handler(httpd_req_t *req) {
  const char* html = "<html><body><img src=\"/stream\"></body></html>";
  httpd_resp_send(req, html, strlen(html));
  return ESP_OK;
}

#define PART_BOUNDARY "123456789000000000000987654321"
static const char *_STREAM_CONTENT_TYPE = "multipart/x-mixed-replace;boundary=" PART_BOUNDARY;
static const char *_STREAM_BOUNDARY = "\r\n--" PART_BOUNDARY "\r\n";
static const char *_STREAM_PART = "Content-Type: image/jpeg\r\nContent-Length: %u\r\nX-Timestamp: %d.%06d\r\n\r\n";
esp_err_t streamHandler(httpd_req_t *req) {
  camera_fb_t *fb = NULL;
  struct timeval _timestamp;
  esp_err_t res = ESP_OK;
  size_t _jpg_buf_len = 0;
  uint8_t *_jpg_buf = NULL;
  char *part_buf[128];

  res = httpd_resp_set_type(req, _STREAM_CONTENT_TYPE);
  if (res != ESP_OK) {
    return res;
  }

  httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
  httpd_resp_set_hdr(req, "X-Framerate", "60");

  while (true) {
    fb = esp_camera_fb_get();
    if (!fb) {
      Serial.println("Camera capture failed");
      res = ESP_FAIL;
    } else {
      _timestamp.tv_sec = fb->timestamp.tv_sec;
      _timestamp.tv_usec = fb->timestamp.tv_usec;
      if (fb->format != PIXFORMAT_JPEG) {
        bool jpeg_converted = frame2jpg(fb, 80, &_jpg_buf, &_jpg_buf_len);
        esp_camera_fb_return(fb);
        fb = NULL;
        if (!jpeg_converted) {
          Serial.println("JPEG compression failed");
          res = ESP_FAIL;
        }
      } else {
        _jpg_buf_len = fb->len;
        _jpg_buf = fb->buf;
      }
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, _STREAM_BOUNDARY, strlen(_STREAM_BOUNDARY));
    }
    if (res == ESP_OK) {
      size_t hlen = snprintf((char *)part_buf, 128, _STREAM_PART, _jpg_buf_len, _timestamp.tv_sec, _timestamp.tv_usec);
      res = httpd_resp_send_chunk(req, (const char *)part_buf, hlen);
    }
    if (res == ESP_OK) {
      res = httpd_resp_send_chunk(req, (const char *)_jpg_buf, _jpg_buf_len);
    }
    if (fb) {
      esp_camera_fb_return(fb);
      fb = NULL;
      _jpg_buf = NULL;
    } else if (_jpg_buf) {
      free(_jpg_buf);
      _jpg_buf = NULL;
    }
    if (res != ESP_OK) {
      Serial.println("Send frame failed");
      break;
    }
  }

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

  String json = "[";
  for (int i = 0; i < res.len; i++) {
    json += "{\"sensor_id\":" + String(i) + ",\"value\":" + String(res.data[i], 0) + "}";
    if (i < res.len - 1) {
      json += ",";
    }
  }
  json += "]";

  esp_err_t err = httpd_resp_send(req, json.c_str(), strlen(json.c_str()));

  return err;
}

esp_err_t readTemperatureHandler(httpd_req_t *req) {
  sendCommand(READ_TEMPERATURE);

  unsigned long start = millis();
  while (!isResponseReceived() && millis() - start < 5000);
  if (millis() - start >= 5000) {
    return ESP_ERR_TIMEOUT;
  }

  httpd_resp_set_type(req, "application/json");
  response res = getResponse();

  float temperature = res.data[0];

  String json = "{";
  json += "\"temperature\":" + String(temperature, 2);
  json += "}";

  esp_err_t err = httpd_resp_send(req, json.c_str(), strlen(json.c_str()));

  return err;
}

esp_err_t readHumidityHandler(httpd_req_t *req) {
  sendCommand(READ_HUMIDITY);

  unsigned long start = millis();
  while (!isResponseReceived() && millis() - start < 5000);
  if (millis() - start >= 5000) {
    return ESP_ERR_TIMEOUT;
  }

  httpd_resp_set_type(req, "application/json");
  response res = getResponse();

  float humidity = res.data[0];

  String json = "{";
  json += "\"humidity\":" + String(humidity, 2);
  json += "}";

  esp_err_t err = httpd_resp_send(req, json.c_str(), strlen(json.c_str()));

  return err;
}

esp_err_t readLightHandler(httpd_req_t *req) {
  sendCommand(READ_LIGHT);

  unsigned long start = millis();
  while (!isResponseReceived() && millis() - start < 5000);
  if (millis() - start >= 5000) {
    return ESP_ERR_TIMEOUT;
  }

  httpd_resp_set_type(req, "application/json");
  response res = getResponse();

  float light = res.data[0];

  String json = "{";
  json += "\"light\":" + String(light, 0);
  json += "}";

  esp_err_t err = httpd_resp_send(req, json.c_str(), strlen(json.c_str()));

  return err;
}

esp_err_t readStatsHandler(httpd_req_t *req) {
  String json = "{";

  // Get moisture levels
  sendCommand(READ_SOIL_MOISTURE);
  unsigned long start = millis();
  while (!isResponseReceived() && millis() - start < 5000);
  if (millis() - start >= 5000) {
    return ESP_ERR_TIMEOUT;
  }
  response res = getResponse();

  json += "\"moisture\": [";
  for (int i = 0; i < res.len; i++) {
    json += "{\"sensor_id\":" + String(i) + ",\"value\":" + String(res.data[i], 0) + "}";
    if (i < res.len - 1) {
      json += ",";
    }
  }
  json += "],";

  // Get temperature levels
  sendCommand(READ_TEMPERATURE);
  start = millis();
  while (!isResponseReceived() && millis() - start < 5000);
  if (millis() - start >= 5000) {
    return ESP_ERR_TIMEOUT;
  }
  res = getResponse();

  float temperature = res.data[0];
  json += "\"temperature\":" + String(temperature, 2) + ",";

  // Get humidity levels
  sendCommand(READ_HUMIDITY);
  start = millis();
  while (!isResponseReceived() && millis() - start < 5000);
  if (millis() - start >= 5000) {
    return ESP_ERR_TIMEOUT;
  }
  res = getResponse();

  float humidity = res.data[0];
  json += "\"humidity\":" + String(humidity, 2) + ",";

  // Get light levels
  sendCommand(READ_LIGHT);
  start = millis();
  while (!isResponseReceived() && millis() - start < 5000);
  if (millis() - start >= 5000) {
    return ESP_ERR_TIMEOUT;
  }
  res = getResponse();

  float light = res.data[0];
  json += "\"light\":" + String(light, 0);
  json += "}";

  httpd_resp_set_type(req, "application/json");
  esp_err_t err = httpd_resp_send(req, json.c_str(), strlen(json.c_str()));

  return err;
}

// Uri definitions
httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = captureHandler,
    .user_ctx = NULL
};

httpd_uri_t stream_uri = {
    .uri = "/stream",
    .method = HTTP_GET,
    .handler = streamHandler,
    .user_ctx = NULL
};

httpd_uri_t stream_test_uri = {
    .uri = "/stream_test",
    .method = HTTP_GET,
    .handler = index_handler,
    .user_ctx = NULL
};

httpd_uri_t moisture_uri = {
    .uri = "/actuator/moisture",
    .method = HTTP_GET,
    .handler = readMoistureHandler,
    .user_ctx = NULL
};

httpd_uri_t temperature_uri = {
    .uri = "/actuator/temperature",
    .method = HTTP_GET,
    .handler = readTemperatureHandler,
    .user_ctx = NULL
};

httpd_uri_t humidity_uri = {
    .uri = "/actuator/humidity",
    .method = HTTP_GET,
    .handler = readHumidityHandler,
    .user_ctx = NULL
};

httpd_uri_t light_uri = {
    .uri = "/actuator/light",
    .method = HTTP_GET,
    .handler = readHumidityHandler,
    .user_ctx = NULL
};

httpd_uri_t stats_uri = {
    .uri = "/stats",
    .method = HTTP_GET,
    .handler = readStatsHandler,
    .user_ctx = NULL
};

server_err_t startServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
  
    if (httpd_start(&server, &config) == ESP_OK) {
      httpd_register_uri_handler(server, &capture_uri);
      httpd_register_uri_handler(server, &stream_uri);
      httpd_register_uri_handler(server, &stream_test_uri);

      httpd_register_uri_handler(server, &moisture_uri);
      httpd_register_uri_handler(server, &temperature_uri);
      httpd_register_uri_handler(server, &humidity_uri);
      httpd_register_uri_handler(server, &light_uri);
      httpd_register_uri_handler(server, &stats_uri);

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