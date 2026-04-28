typedef struct {
  int timeout_ms;  // Timeout in milliseconds
} esp_task_wdt_config_t;

void esp_task_wdt_init(const esp_task_wdt_config_t* config);
void esp_task_wdt_add(void* task_handle);
//void esp_task_wdt_delete(void* task_handle);
void esp_task_wdt_reset();
