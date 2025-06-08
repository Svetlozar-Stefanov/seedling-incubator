#include "actuator_slave.h"

#include "soil_moisture.h"
#include "temperature_humidity.h"

#include <Arduino.h>

#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_now.h>

uint8_t masterMac[] = {0xec, 0xe3, 0x34, 0xb3, 0x4a, 0xf8};

struct received {
  command cmd;
} rec;

struct response {
  float data[64];
  int len;
  command cmd;
} res;

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

void onResponseSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Successful" : "Delivery Failed");
}

void OnRequestReceived(const uint8_t * mac, const uint8_t *data, int len) {
  memcpy(&rec, data, sizeof(rec));
  Serial.printf("Command received: %d\n", rec.cmd);

  if (rec.cmd == READ_SOIL_MOISTURE) {
    const int * moisture = getMoistureMeasurements();

    res.cmd = READ_SOIL_MOISTURE;
    for (int i = 0; i < NUMBER_OF_SOIL_MOISTURE_SENSORS; i++) {
      res.data[i] = moisture[i];
    }
    res.len = NUMBER_OF_SOIL_MOISTURE_SENSORS;

    // Return moisture data
    esp_err_t err = esp_now_send(masterMac, (uint8_t *) &res, sizeof(res));
    if (err != ESP_OK) {
      Serial.printf("ERROR: Command not sent sucessfully. %s\n", 
        esp_err_to_name(err));
    }
  } else if (rec.cmd == READ_TEMPERATURE) {
    float temperature = getTemperature();
    
    res.cmd = READ_TEMPERATURE;
    res.data[0] = temperature;
    res.len = 1;

    // Return temperature data
    esp_err_t err = esp_now_send(masterMac, (uint8_t *) &res, sizeof(res));
    if (err != ESP_OK) {
      Serial.printf("ERROR: Command not sent sucessfully. %s\n", 
        esp_err_to_name(err));
    }
  }
  else if (rec.cmd == READ_HUMIDITY) {
    float humidity = getAirHumidity();
    
    res.cmd = READ_HUMIDITY;
    res.data[0] = humidity;
    res.len = 1;

    // Return humidity data
    esp_err_t err = esp_now_send(masterMac, (uint8_t *) &res, sizeof(res));
    if (err != ESP_OK) {
      Serial.printf("ERROR: Command not sent sucessfully. %s\n", 
        esp_err_to_name(err));
    }
  }
}

actuator_err_t setupActuatorSlave() {
  if (esp_now_init() != ESP_OK) {
   return ACTUATOR_ERR_INITIALIZATION_FAIL;
  }

  esp_now_register_send_cb(onResponseSent);
  esp_now_register_recv_cb(esp_now_recv_cb_t(OnRequestReceived));

  memcpy(peerInfo.peer_addr, masterMac, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;

  if (!esp_now_add_peer(&peerInfo)) {
    return ACTUATOR_ERR_FAILED_TO_CONNECT_TO_MASTER;
  }

  return ACTUATOR_SUCCESS;
}

const char * getActuatorErrorMessage(actuator_err_t err) {
  switch (err) {
    case ACTUATOR_SUCCESS: return "Success";
    case ACTUATOR_ERR_INITIALIZATION_FAIL: return "Initialization failed";
    case ACTUATOR_ERR_FAILED_TO_READ_MAC_ADDRESS: return "Failed to get MAC Address";
    case ACTUATOR_ERR_FAILED_TO_CONNECT_TO_MASTER: return "Failed to connect to master";
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
