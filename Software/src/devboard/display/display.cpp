// A realtime display of battery status and events, using a I2C-connected 128x64
// OLED display based on the SSD1306 driver.

#include "../../battery/BATTERIES.h"
#include "../../datalayer/datalayer.h"
#include "../hal/hal.h"
#include "../utils/events.h"
#include "../utils/logging.h"
#include "fonts.h"

#include "Arduino.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

#define I2C_MASTER_FREQ_HZ 1000000  // Use a ridiculously fast I2C speed (seems to work!)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))

i2c_master_bus_handle_t bus_handle;
i2c_master_dev_handle_t dev_handle;
bool display_initialized = false;
unsigned long lastUpdateMillis = 0;
static std::vector<EventData> order_events;
int num_batteries = 1;

static esp_err_t i2c_write(const uint8_t* data, size_t len) {
  return i2c_master_transmit(dev_handle, data, len, 1000 / portTICK_PERIOD_MS);
}

static void i2c_data(const uint8_t* data, size_t len) {
  uint8_t write_buf[1025];
  write_buf[0] = 0x40;  // Control byte 0x40 for data
  memcpy(&write_buf[1], data, len);
  i2c_write(write_buf, len + 1);
}

static void set_ram_pointer(int x, int y) {
  uint8_t cmd[] = {
      0x80, (uint8_t)(0x00 | (x & 0x0F)),         // Set lower column address
      0x80, (uint8_t)(0x10 | ((x >> 4) & 0x0F)),  // Set higher column address
      0x00, (uint8_t)(0xB0 | (y & 0x0F)),         // Set page address
  };
  i2c_write(cmd, sizeof(cmd));
}

static void write_text(int x, int y, const char* str, bool invert) {
  set_ram_pointer(x, y);

  while (*str) {
    char c = *str++;
    if (c < ' ' || c > '~') {
      c = '!';  // Replace unsupported characters
    }

    uint8_t data[6];
    if (invert) {
      for (int i = 0; i < 6; i++) {
        data[i] = ~font6x8_basic[c - ' '][i];
      }
      i2c_data(data, 6);
    } else {
      i2c_data(font6x8_basic[c - ' '], 6);
    }
  }
}

/*
static void write_big_text(int x, int y, const char* str, bool invert) {
  for (int r = 0; r < 2; r++) {
    set_ram_pointer(x, y + r);

    char* ptr = (char*)str;
    while (*ptr) {
      char c = *ptr++;
      if (c < ' ' || c > '~') {
        c = '!';  // Replace unsupported characters
      }

      char slice[16];
      for (int col = 0; col < 6; col++) {
        uint8_t byte = font6x8_basic[c - ' '][col];

        byte = r == 0 ? (byte & 0xf) : (byte >> 4);
        byte = (byte | (byte << 2)) & 0x33;
        byte = (byte | (byte << 1)) & 0x55;
        byte *= 0x03;

        if(invert) {
          byte = ~byte;
        }

        slice[col * 2] = byte;
        slice[col * 2 + 1] = byte;
      }

      i2c_data((uint8_t*)slice, 12);
    }
  }
}
*/

static void write_tall_text(int x, int y, const char* str, bool invert) {
  for (int r = 0; r < 2; r++) {
    set_ram_pointer(x, y + r);

    char* ptr = (char*)str;
    while (*ptr) {
      char c = *ptr++;
      if (c < ' ' || c > '9') {
        c = '!';  // Replace unsupported characters
      }

      char slice[8];
      for (int col = 0; col < 8; col++) {
        uint8_t byte = font8x8_basic[c - ' '][col];

        byte = r == 0 ? (byte & 0xf) : (byte >> 4);
        byte = (byte | (byte << 2)) & 0x33;
        byte = (byte | (byte << 1)) & 0x55;
        byte *= 0x03;

        if (invert) {
          byte = ~byte;
        }

        slice[col] = byte;
      }

      i2c_data((uint8_t*)slice, 8);
    }
  }
}

static void clear() {
  uint8_t blank[128] = {
      0,
  };
  for (int i = 0; i < 8; i++) {
    set_ram_pointer(0, i);
    i2c_data(blank, sizeof(blank));
  }
}

void init_display() {
  auto display_sda = esp32hal->DISPLAY_SDA_PIN();
  auto display_scl = esp32hal->DISPLAY_SCL_PIN();

  if (display_sda == GPIO_NUM_NC || display_scl == GPIO_NUM_NC) {
    return;
  }

  if (!esp32hal->alloc_pins("I2C Display", display_sda, display_scl)) {
    logging.printf("Failed to allocate pins for I2C Display\n");
    return;
  }

  i2c_master_bus_config_t bus_config = {
      .i2c_port = -1,  // autoselect
      .sda_io_num = display_sda,
      .scl_io_num = display_scl,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .intr_priority = 0,
      .trans_queue_depth = 0,
      .flags =
          {
              .enable_internal_pullup = true,
              .allow_pd = false,
          },
  };
  ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, &bus_handle));

  i2c_device_config_t dev_config = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = 0x3c,
      .scl_speed_hz = I2C_MASTER_FREQ_HZ,
      .scl_wait_us = 0,
      .flags = {},
  };
  ESP_ERROR_CHECK(i2c_master_bus_add_device(bus_handle, &dev_config, &dev_handle));

  static const uint8_t init[] = {
      0xae,    // display off
      0xd5,    // set display clock divider
      0x80,    //   /1, middle freq
      0xa8,    // set multiplex
      64 - 1,  //   height - 1

      0xd3,  // set display offset
      0x00,  //   no offset
      0x40,  // set start line at 0

      0x8d,  // charge pump
      0x14,  // enable

      0x20,  // memory mode
      0x00,  //   horizontal addressing
      0xa1,  // seg remap = 1 (mirror x)
      0xc8,  // com scan = dec

      0x81,  // contrast
      0x0f,  //   0x0f/0xff

      0xaf,  // display on
  };
  uint8_t buf[sizeof(init) * 2];
  for (size_t i = 0; i < sizeof(init); i++) {
    buf[i * 2] = 0x80;  // Control byte for command
    buf[i * 2 + 1] = init[i];
  }
  if (i2c_write(buf, sizeof(buf)) != ESP_OK) {
    logging.printf("Failed to initialize I2C Display\n");
    return;
  }

  clear();
  display_initialized = true;

  // Count configured batteries
  if (battery2)
    num_batteries++;
  if (battery3)
    num_batteries++;
}

static void printn(char* buf, int value, int digits) {
  bool neg = value < 0;
  if (neg)
    value = -value;
  for (int i = digits - 1; i >= 0; i--) {
    buf[i] = '0' + (value % 10);
    if (value < 10) {
      if (neg && i > 0)
        buf[i - 1] = '-';
      break;
    }
    value = value / 10;
  }
}

inline void print2(char* buf, int value) {
  printn(buf, value, 2);
}

inline void print3(char* buf, int value) {
  printn(buf, value, 3);
}

inline void print4(char* buf, int value) {
  printn(buf, value, 4);
}

static void print3d1(char* buf, int value) {
  print3(buf, value / 10);

  buf[3] = '.';
  buf[4] = '0' + (abs(value) % 10);
}

static void print_interval(char* buf, uint64_t elapsed) {
  if (elapsed < 100000) {
    print2(buf, int(elapsed / 1000));
    buf[2] = 's';
  } else if (elapsed < 6000000) {
    print2(buf, int(elapsed / 60000));
    buf[2] = 'm';
  } else if (elapsed < 360000000) {
    print2(buf, int(elapsed / 3600000));
    buf[2] = 'h';
  } else {
    print2(buf, MIN(99, int(elapsed / 86400000)));
    buf[2] = 'd';
  }
}

static void cpy(char* dst, const char* src) {
  while (*src && *dst) {
    *dst++ = *src++;
  }
}

static void print_battery_status(int row, DATALAYER_BATTERY_STATUS_TYPE& status, int num, int page) {
  char buf[22];
  memset(buf, ' ', sizeof(buf));

  print3(buf, status.reported_soc / 100);
  buf[3] = '%';
  buf[4] = '\0';
  write_tall_text(0, row, buf, false);

  if (num_batteries > 1) {
    buf[6] = 'B';
    buf[7] = 'a';
    buf[8] = 't';
    buf[9] = '0' + num;
  }
  printn(buf + 14, status.active_power_W, 6);
  buf[20] = 'W';
  buf[21] = '\0';
  write_text(6 * 6, row++, buf + 6, false);

  memset(buf, ' ', sizeof(buf));

  if (page == 0) {
    // First page, voltage + remaining capacity

    print3d1(buf + 6, status.voltage_dV);
    buf[11] = 'V';

    print3d1(buf + 13, status.remaining_capacity_Wh / 100);
    buf[18] = 'k';
    buf[19] = 'W';
    buf[20] = 'h';
    buf[21] = '\0';
  } else if (page == 1) {
    // Second page, cell delta + current

    print4(buf + 6, status.cell_max_voltage_mV - status.cell_min_voltage_mV);
    buf[10] = 'm';
    buf[11] = 'V';
    buf[12] = ' ';
    print3d1(buf + 15, status.current_dA);
    buf[20] = 'A';
    buf[21] = '\0';
  } else if (page == 2) {
    // Third page, cell voltage range

    print4(buf + 6, status.cell_min_voltage_mV);
    buf[10] = 'm';
    buf[11] = 'V';
    buf[13] = '/';
    print4(buf + 15, status.cell_max_voltage_mV);
    buf[19] = 'm';
    buf[20] = 'V';
    buf[21] = '\0';
  } else if (page == 3) {
    // Foutrh page, temperature range

    print3d1(buf + 6, status.temperature_min_dC);
    buf[11] = 'c';
    buf[13] = '/';
    print3d1(buf + 15, status.temperature_max_dC);
    buf[20] = 'c';
    buf[21] = '\0';
  }

  write_text(6 * 6, row, buf + 6, false);
}

static const int PRE_SCROLL = 4;   // How long to wait before scrolling
static const int POST_SCROLL = 4;  // How long to wait after scrolling
int scroll_x = 0;

static void print_events(int row, int count) {
  char buf[22];

  // Collect all events
  order_events.clear();
  for (int i = 0; i < EVENT_NOF_EVENTS; i++) {
    auto event_pointer = get_event_pointer((EVENTS_ENUM_TYPE)i);
    if (event_pointer->occurences > 0 && event_pointer->level > EVENT_LEVEL_INFO) {
      order_events.push_back({static_cast<EVENTS_ENUM_TYPE>(i), event_pointer});
    }
  }
  // Sort events by timestamp
  std::sort(order_events.begin(), order_events.end(), compareEventsByTimestampDesc);
  uint64_t current_timestamp = millis64();
  int longest_event_str = 0;

  int i;
  for (i = 0; i < order_events.size() && i < count; i++) {
    memset(buf, ' ', sizeof(buf));

    auto ev = order_events[i];
    uint64_t elapsed = MAX((int64_t)(current_timestamp - ev.event_pointer->timestamp), 0);
    print_interval(buf, elapsed);

    const char* event_str = get_event_enum_string(ev.event_handle);
    int event_str_len = strlen(event_str);
    longest_event_str = MAX(longest_event_str, event_str_len);

    buf[21] = '\0';
    cpy(buf + 4, event_str + MIN(MAX(scroll_x - PRE_SCROLL, 0), MAX(event_str_len - 17, 0)));

    // Error-level events are highlighted
    write_text(0, i + row, buf, get_event_level() == EVENT_LEVEL_ERROR && ev.event_pointer->level == EVENT_LEVEL_ERROR);
  }

  // Clear remaining lines
  memset(buf, ' ', sizeof(buf));
  buf[21] = '\0';
  for (; i < count; i++) {
    write_text(0, i + row, buf, false);
  }

  // Update scrolling (if event names are too long)
  if (longest_event_str > 17) {
    scroll_x++;
    if (scroll_x > longest_event_str - 17 + PRE_SCROLL + POST_SCROLL) {
      scroll_x = 0;
    }
  } else {
    scroll_x = 0;
  }
}

static void print_wifi_status(int row) {
  wl_status_t status = WiFi.status();
  char buf[22];
  memset(buf, ' ', sizeof(buf));
  buf[21] = '\0';

  if (status == WL_CONNECTED) {
    cpy(buf, WiFi.localIP().toString().c_str());
    print3(buf + 16, WiFi.RSSI());
    buf[19] = 'd';
    buf[20] = 'B';
  }

  write_text(0, row, buf, false);
}

void update_display() {
  if (!display_initialized) {
    return;
  }

  // We update the display every 500ms
  auto currentMillis = millis();
  if (currentMillis - lastUpdateMillis < 500) {
    return;
  }

  // We cycle through several pages of battery data
  const int NUM_PAGES = 4;
  const int PAGE_TIME = 3;
  static int phase = 0;

  // Calculate total phases: NUM_PAGES per battery, PAGE_TIME seconds each
  int total_phases = NUM_PAGES * num_batteries * PAGE_TIME * 2;  // *2 for 500ms updates

  int current_phase = phase / (PAGE_TIME * 2);
  int battery_index = current_phase % num_batteries;
  int page = (current_phase / num_batteries) % NUM_PAGES;

  // Print the battery status for current battery
  if (battery_index == 0) {
    print_battery_status(0, datalayer.battery.status, 1, page);
  } else if (battery_index == 1) {
    print_battery_status(0, datalayer.battery2.status, 2, page);
  } else {
    print_battery_status(0, datalayer.battery3.status, 3, page);
  }

  write_text(0, 2, "---------------------", false);

  // Print the events below
  print_events(3, 3);

  write_text(0, 6, "---------------------", false);

  // Then IP/RSSI at the bottom
  print_wifi_status(7);

  phase++;
  if (phase >= total_phases) {
    phase = 0;
  }
  lastUpdateMillis = currentMillis;
}
