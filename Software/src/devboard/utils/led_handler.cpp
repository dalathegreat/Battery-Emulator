#include "led_handler.h"
#include "../../datalayer/datalayer.h"
#include "../../devboard/hal/hal.h"
#include "events.h"
#include "value_mapping.h"

#define COLOR_GREEN(x) (((uint32_t)0 << 16) | ((uint32_t)x << 8) | 0)
#define COLOR_YELLOW(x) (((uint32_t)x << 16) | ((uint32_t)x << 8) | 0)
#define COLOR_RED(x) (((uint32_t)x << 16) | ((uint32_t)0 << 8) | 0)
#define COLOR_BLUE(x) (((uint32_t)0 << 16) | ((uint32_t)0 << 8) | x)

#define BPM_TO_MS(x) ((60000 / (x)))  // 60 * 1000 = 60000

#define HEARTBEAT_BASE 150      // 0.15 * 1000
#define HEARTBEAT_PEAK1 800     // 0.80 * 1000
#define HEARTBEAT_PEAK2 550     // 0.55 * 1000
#define HEARTBEAT_DEVIATION 50  // 0.05 * 1000
#define SCALE_FACTOR 1000

static LED* led = nullptr;

static bool led_override_active = false;
static uint32_t led_override_color = 0;
static uint16_t led_override_period_ms = 0;

void set_led_override(bool active, uint32_t color, uint16_t period_ms) {
  if (led == nullptr) {
    return;  // no LED (GPIO unset or used for an OLED) — nothing to drive
  }
  led_override_active = active;
  led_override_color = color;
  led_override_period_ms = period_ms;
}

uint16_t map_int(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool led_init(void) {
  gpio_num_t led_pin = esp32hal->LED_PIN();

  if (led_pin == GPIO_NUM_NC) {
    return true;
  }

  if (!esp32hal->alloc_pins("LED", led_pin)) {
    DEBUG_PRINTF("LED setup failed\n");
    return false;
  }

  led = new LED(datalayer.battery.status.led_mode, led_pin, esp32hal->LED_MAX_BRIGHTNESS());

  return true;
}

void led_exe(void) {
  if (led == nullptr) {
    return;
  }

  led->exe();
}

void LED::exe(void) {
  // Button-hold feedback: square-wave blink at full brightness, ahead of normal state.
  if (led_override_active && led_override_period_ms > 0) {
    const bool on = (millis() / (led_override_period_ms / 2)) % 2;
    pixels.setPixelColor(on ? led_override_color : 0);
    pixels.show();
    return;
  }

  // Update brightness
  switch (datalayer.battery.status.led_mode) {
    case led_mode_enum::FLOW:
      flow_run();
      break;
    case led_mode_enum::HEARTBEAT:
      heartbeat_run();
      break;
#ifdef HW_LILYGO2CAN
    case led_mode_enum::GRB_FLOW:
      pixels.setColorOrder(GRB);
      flow_run();
      break;
    case led_mode_enum::GRB_HEARTBEAT:
      pixels.setColorOrder(GRB);
      heartbeat_run();
      break;
    case led_mode_enum::GRB_CLASSIC:
      pixels.setColorOrder(GRB);
      classic_run();
      break;
#endif
    case led_mode_enum::CLASSIC:
    default:
      classic_run();
      break;
  }

  // Set color
  switch (get_emulator_status()) {
    case EMULATOR_STATUS::STATUS_OK:
      pixels.setPixelColor(COLOR_GREEN(brightness));  // Green pulsing LED
      break;
    case EMULATOR_STATUS::STATUS_WARNING:
      pixels.setPixelColor(COLOR_YELLOW(brightness));  // Yellow pulsing LED
      break;
    case EMULATOR_STATUS::STATUS_ERROR:
      pixels.setPixelColor(COLOR_RED(esp32hal->LED_MAX_BRIGHTNESS()));  // Red LED full brightness
      break;
    case EMULATOR_STATUS::STATUS_UPDATING:
      pixels.setPixelColor(COLOR_BLUE(brightness));  // Blue pulsing LED
      break;
  }

  pixels.show();  // This sends the updated pixel color to the hardware.
}

void LED::classic_run(void) {
  // Determine how bright the LED should be
  brightness = up_down(500);
}

void LED::flow_run(void) {
  // Determine how bright the LED should be
  if (datalayer.battery.status.active_power_W < -50) {
    // Discharging
    brightness = max_brightness - up_down(950);
  } else if (datalayer.battery.status.active_power_W > 50) {
    // Charging
    brightness = up_down(950);
  } else {  // Idle
    brightness = up_down(500);
  }
}

void LED::heartbeat_run(void) {
  uint16_t period;
  switch (get_event_level()) {
    case EVENT_LEVEL_WARNING:
      period = BPM_TO_MS(70);
      break;
    case EVENT_LEVEL_ERROR:
      period = BPM_TO_MS(100);
      break;
    default:
      period = BPM_TO_MS(35);
      break;
  }

  uint16_t ms = millis() % period;

  // Calculate percentage as integer (0-1000 represents 0.0-1.0)
  uint16_t period_pct = (ms * SCALE_FACTOR) / period;
  uint16_t brightness_scaled;

  if (period_pct < 100) {  // 0.10 * 1000
    brightness_scaled = map_int(period_pct, 0, 100, HEARTBEAT_BASE, HEARTBEAT_BASE - HEARTBEAT_DEVIATION);
  } else if (period_pct < 200) {  // 0.20 * 1000
    brightness_scaled =
        map_int(period_pct, 100, 200, HEARTBEAT_BASE - HEARTBEAT_DEVIATION, HEARTBEAT_BASE - HEARTBEAT_DEVIATION * 2);
  } else if (period_pct < 250) {  // 0.25 * 1000
    brightness_scaled = map_int(period_pct, 200, 250, HEARTBEAT_BASE - HEARTBEAT_DEVIATION * 2, HEARTBEAT_PEAK1);
  } else if (period_pct < 300) {  // 0.30 * 1000
    brightness_scaled = map_int(period_pct, 250, 300, HEARTBEAT_PEAK1, HEARTBEAT_BASE - HEARTBEAT_DEVIATION);
  } else if (period_pct < 400) {  // 0.40 * 1000
    brightness_scaled = map_int(period_pct, 300, 400, HEARTBEAT_BASE - HEARTBEAT_DEVIATION, HEARTBEAT_PEAK2);
  } else if (period_pct < 550) {  // 0.55 * 1000
    brightness_scaled = map_int(period_pct, 400, 550, HEARTBEAT_PEAK2, HEARTBEAT_BASE + HEARTBEAT_DEVIATION * 2);
  } else {
    brightness_scaled =
        map_int(period_pct, 550, SCALE_FACTOR, HEARTBEAT_BASE + HEARTBEAT_DEVIATION * 2, HEARTBEAT_BASE);
  }

  // Convert scaled brightness (0-1000) to actual brightness (0-255)
  brightness = (brightness_scaled * max_brightness) / SCALE_FACTOR;
}

uint8_t LED::up_down(uint16_t middle_point_scaled) {
  // Convert scaled middle point to actual milliseconds
  uint16_t middle_point = (middle_point_scaled * LED_PERIOD_MS) / SCALE_FACTOR;
  middle_point = CONSTRAIN(middle_point, 0, LED_PERIOD_MS);

  uint16_t ms = millis() % LED_PERIOD_MS;

  if (ms < middle_point) {
    brightness = map_uint16(ms, 0, middle_point, 0, max_brightness);
  } else {
    brightness = max_brightness - map_uint16(ms, middle_point, LED_PERIOD_MS, 0, max_brightness);
  }

  return CONSTRAIN(brightness, 0, max_brightness);
}
