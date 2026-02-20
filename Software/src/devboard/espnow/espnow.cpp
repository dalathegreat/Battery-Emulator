// A realtime display of battery status and events, using a I2C-connected 128x64

#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../hal/hal.h"
#include "../utils/events.h"
#include "../utils/logging.h"
#include <WiFi.h>
#include <esp_now.h>
#include "Arduino.h"
#include "esp_log.h"
#include "espnow.h"
#include "freertos/FreeRTOS.h"

// Create structures for keeping the send data
struct BATTERY_INFO_TYPE b_info;
struct BATTERY_STATUS_TYPE b_status;
struct BATTERY_CELL_STATUS_TYPE b_cells;
struct BATTERY_BALANCING_STATUS_TYPE b_bal_status;
struct ESPNOW_BATTERY_MESSAGE b_message;

// TODO: Add support for configurable MAC address instead of broadcast
uint8_t broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

esp_now_peer_info_t peerInfo;

bool espnow_initialized = false;
unsigned long lastUpdateMillis = 0;
int num_batteries = 1;

// ESPNow callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.print("\r\nLast Packet Send Status:\t");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  if(status != ESP_NOW_SEND_SUCCESS)
  {
    Serial.println("\r\nLast ESPNow Packet Send Status:\tDelivery Fail");
  }
}

static void send_battery_status(DATALAYER_BATTERY_STATUS_TYPE& status, uint8_t b_index) {
  // Set values to send
  memcpy(&b_message.esp_message, &status, 80); //80 bytes without ballancing info
  b_message.emulator_id = 0x1234;
  b_message.battery_id = b_index + 1;
  b_message.esp_message_type = BAT_STATUS;
  
  // Send message via ESPNow
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &b_message, sizeof(struct BATTERY_STATUS_TYPE)+4*sizeof(uint8_t));

  if (result == ESP_OK) {
    // Serial.println("Sent with success");
    ;
  }
  else {
    Serial.println("Error sending the ESPNow Battery Status data");
  }
}

static void send_battery_info(DATALAYER_BATTERY_INFO_TYPE& info, uint8_t b_index) {
  // Set values to send
  memcpy(&b_message.esp_message, &info, 24); //24 bytes 
  b_message.emulator_id = 0x1234;
  b_message.battery_id = b_index + 1;
  b_message.esp_message_type = BAT_INFO;
  
  // Send message via ESPNow
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &b_message, sizeof(struct BATTERY_INFO_TYPE)+4*sizeof(uint8_t));

  if (result == ESP_OK) {
    // Serial.println("Sent with success");
    ;
  }
  else {
    Serial.println("Error sending the ESPNow Battery Info data");
  }
}

static void send_battery_balancing(DATALAYER_BATTERY_INFO_TYPE& info, DATALAYER_BATTERY_STATUS_TYPE& status, uint8_t b_index) {
  // Set values to send
  b_bal_status.number_of_cells = info.number_of_cells;
  for (int i = 0; i < b_bal_status.number_of_cells; i++)
  {
    b_bal_status.cell_balancing_status[i] = status.cell_balancing_status[i];
  }
  memcpy(&b_message.esp_message, &b_bal_status, sizeof(b_bal_status)); //193 bytes 
  b_message.emulator_id = 0x1234;
  b_message.battery_id = b_index + 1;
  b_message.esp_message_type = BAT_BALANCE;
  
  // Send message via ESPNow
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &b_message, sizeof(struct BATTERY_BALANCING_STATUS_TYPE)+4*sizeof(uint8_t));

  if (result == ESP_OK) {
    // Serial.println("Sent with success");
    ;
  }
  else {
    Serial.println("Error sending the ESPNow Battery Balancing data");
  }
}

static void send_battery_cell_status(DATALAYER_BATTERY_INFO_TYPE& info, DATALAYER_BATTERY_STATUS_TYPE& status, uint8_t b_index) {
  // Set values to send
  b_cells.number_of_cells = info.number_of_cells;
  for (int i = 0; i < b_cells.number_of_cells; i++)
  {
    b_cells.cell_voltages_mV[i] = status.cell_voltages_mV[i]/20;
  }
  memcpy(&b_message.esp_message, &b_cells, sizeof(b_cells)); //193 bytes 
  b_message.emulator_id = 0x1234;
  b_message.battery_id = b_index + 1;
  b_message.esp_message_type = BAT_CELL_STATUS;
  
  // Send message via ESPNow
  esp_err_t result = esp_now_send(broadcastAddress, (uint8_t *) &b_message, sizeof(struct BATTERY_CELL_STATUS_TYPE)+4*sizeof(uint8_t));

  if (result == ESP_OK) {
    // Serial.println("Sent with success");
    ;
  }
  else {
    Serial.println("Error sending the ESPNow Cell Status data");
  }
}

void init_espnow() {
  // Check wifi status
  // Wifi Should be already set before initializing ESPNow
  if((WiFi.getMode() != WIFI_AP_STA) && (WiFi.getMode() != WIFI_STA)) {
    Serial.println("Wifi should be initialized before using ESPNow");
    return;
  }

  // Init ESPNow
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESPNow");
    return;
  }

  // Once ESPNow is successfully Init, we will register for Send CB to
  // get the status of Transmitted packet
  ESP_ERROR_CHECK(esp_now_register_send_cb(esp_now_send_cb_t(OnDataSent)));

  // Register peer
  memcpy(peerInfo.peer_addr, broadcastAddress, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  // Add peer        
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add ESPNow peer");
    return;
  }

  // Count configured batteries
  if (battery2)
    num_batteries++;
  if (battery3)
    num_batteries++;

  espnow_initialized = true;
}

void update_espnow() {
  if (!espnow_initialized) {
    return;
  }

  // We send the ESPNow messages every 1000ms
  auto currentMillis = millis();
  if (currentMillis - lastUpdateMillis < 1000) {
    return;
  }

  int battery_index = 0;

  // Print the battery status for current battery
  if (battery_index == 0 && battery != nullptr) {
    send_battery_info(datalayer.battery.info, battery_index);
    send_battery_status(datalayer.battery.status, battery_index);
    send_battery_cell_status(datalayer.battery.info, datalayer.battery.status, battery_index);
    send_battery_balancing(datalayer.battery3.info, datalayer.battery.status, battery_index);

  } else if (battery_index == 1) {
    send_battery_info(datalayer.battery2.info, battery_index);
    send_battery_status(datalayer.battery2.status, battery_index);
    send_battery_cell_status(datalayer.battery2.info, datalayer.battery2.status, battery_index);
    send_battery_balancing(datalayer.battery3.info, datalayer.battery2.status, battery_index);
  } else {
    send_battery_info(datalayer.battery3.info, battery_index);
    send_battery_status(datalayer.battery3.status, battery_index);
    send_battery_cell_status(datalayer.battery3.info, datalayer.battery3.status, battery_index);
    send_battery_balancing(datalayer.battery3.info, datalayer.battery3.status, battery_index);
  }

  lastUpdateMillis = currentMillis;
}