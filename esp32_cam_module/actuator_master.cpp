#include "actuator_master.h"

#include <Arduino.h>

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

uint8_t slaveMac[] = {0x08, 0xb6, 0x1f, 0x71, 0x0c, 0x78};

struct request {
  command cmd;
} req;

struct response res;

bool responseReceived = false;

esp_now_peer_info_t peerInfo;

actuator_err_t printMacAddress() {
    uint8_t baseMac[6];
    esp_err_t ret = esp_wifi_get_mac(WIFI_IF_STA, baseMac);
    if (ret == ESP_OK) {
        Serial.printf("This devices MAC Address is: %02x:%02x:%02x:%02x:%02x:%02x\n",
                    baseMac[0], baseMac[1], baseMac[2],
                    baseMac[3], baseMac[4], baseMac[5]);
        return ACTUATOR_SUCCESS;
    } else {
        Serial.printf("ERROR: Failed to read MAC Address. %s\n",
          esp_err_to_name(ret));
        return ACTUATOR_ERR_FAILED_TO_READ_MAC_ADDRESS;
    }
}

void onRequestSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Successful" : "Delivery Failed");
}

void onResponseReceived(const uint8_t * mac, const uint8_t *data, int len) {
  memcpy(&res, data, sizeof(res));
  responseReceived = true;
}

actuator_err_t connectToActuatorSlave() {
  if (esp_now_init() != ESP_OK) {
   return ACTUATOR_ERR_INITIALIZATION_FAIL;
  }

  esp_now_register_send_cb(onRequestSent);
  esp_now_register_recv_cb(esp_now_recv_cb_t(onResponseReceived));

  memcpy(peerInfo.peer_addr, slaveMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_add_peer(&peerInfo)) {
   return ACTUATOR_ERR_FAILED_TO_CONNECT_TO_SLAVE;
  }
}

command_err_t sendCommand(command cmd) {
  req.cmd = cmd;

  responseReceived = false;
  esp_err_t err = esp_now_send(slaveMac, (uint8_t *) &req, sizeof(req));
  if (err != ESP_OK) {
    Serial.printf("ERROR: Command not sent sucessfully. %s\n", 
      esp_err_to_name(err));
    return COMMAND_ERR_SEND_UNSUCCESSFUL;
  }

  return COMMAND_SUCCESS;
}

bool isResponseReceived() {
  return responseReceived;
}

response getResponse() {
  return res;
}

const char * getActuatorErrorMessage(actuator_err_t err) {
  switch (err) {
    case ACTUATOR_SUCCESS: return "Success";
    case ACTUATOR_ERR_INITIALIZATION_FAIL: return "Initialization failed";
    case ACTUATOR_ERR_FAILED_TO_READ_MAC_ADDRESS: return "Failed to get MAC Address";
    case ACTUATOR_ERR_FAILED_TO_CONNECT_TO_SLAVE: return "Failed to connect to slave";
    default: return "Unknown error";
  }
}

const char* getCommandErrorMessage(command_err_t err) {
  switch (err) {
    case COMMAND_SUCCESS: return "Success";
    case COMMAND_ERR_SEND_UNSUCCESSFUL: return "Command was not sent sucessfully";
    default: return "Unknown error";
  }
}
