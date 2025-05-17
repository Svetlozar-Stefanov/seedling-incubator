#include "server.h"

#include "esp_camera.h"
#include "esp_http_server.h"

httpd_handle_t server = NULL;

esp_err_t capture_handler(httpd_req_t *req) {
    camera_fb_t *fb = esp_camera_fb_get();
    if (!fb) {
      printf("Camera capture failed\n");
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

// Uri definitions
httpd_uri_t capture_uri = {
    .uri = "/capture",
    .method = HTTP_GET,
    .handler = capture_handler,
    .user_ctx = NULL
};

server_err_t startServer() {
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 80;
  
    if (httpd_start(&server, &config) == ESP_OK) {
      httpd_register_uri_handler(server, &capture_uri);
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