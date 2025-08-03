#include <mqtt_client.h>

struct esp_mqtt_client {
  // Dummy structure to represent the MQTT client
};

esp_mqtt_client_handle_t esp_mqtt_client_init(const esp_mqtt_client_config_t* config) {
  return new esp_mqtt_client;
}

esp_err_t esp_mqtt_client_register_event(esp_mqtt_client_handle_t client, esp_mqtt_event_id_t event,
                                         esp_event_handler_t event_handler, void* event_handler_arg) {
  return ESP_OK;
}

esp_err_t esp_mqtt_client_start(esp_mqtt_client_handle_t client) {
  return ESP_OK;
}

int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char* topic, const char* data, int len, int qos,
                            int retain) {
  return 0;
}

int esp_mqtt_client_subscribe_single(esp_mqtt_client_handle_t client, const char* topic, int qos) {
  return 0;
}
