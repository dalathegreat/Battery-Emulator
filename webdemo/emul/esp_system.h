#define ESP_RST_POWERON 0
#define ESP_RST_CPU_LOCKUP 1
#define ESP_RST_PWR_GLITCH 2
#define ESP_RST_USB 3
#define ESP_RST_EFUSE 4
#define ESP_RST_JTAG 5
#define ESP_RST_DEEPSLEEP 6
#define ESP_RST_BROWNOUT 7
#define ESP_RST_SDIO 8
#define ESP_RST_WDT 9
#define ESP_RST_TASK_WDT 10
#define ESP_RST_INT_WDT 11
#define ESP_RST_UNKNOWN 12
#define ESP_RST_EXT 13
#define ESP_RST_SW 14
#define ESP_RST_PANIC 15

typedef int esp_reset_reason_t;

inline esp_reset_reason_t esp_reset_reason() {
  return ESP_RST_POWERON;
}
