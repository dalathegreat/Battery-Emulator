typedef struct {
  uint8_t ssid[32];
  uint8_t password[64];
  struct {
    int ssid_len;
    char* ssid;
  } ap;
} wifi_config_t;

#define WIFI_IF_AP 0

static inline int esp_wifi_get_config(int ifx, wifi_config_t* config) {
  (void)ifx;
  // Return a dummy config for testing
  const char* dummy_ssid = "TestSSID";
  const char* dummy_password = "TestPassword";
  strncpy((char*)config->ssid, dummy_ssid, sizeof(config->ssid));
  strncpy((char*)config->password, dummy_password, sizeof(config->password));
  return 0;  // ESP_OK
}
