#include "led_handler.h"
#include "../../datalayer/datalayer.h"
#include "../../include.h"
#include "events.h"
#include "timer.h"
#include "value_mapping.h"

#define COLOR_GREEN(x) (((uint32_t)0 << 16) | ((uint32_t)x << 8) | 0)
#define COLOR_YELLOW(x) (((uint32_t)x << 16) | ((uint32_t)x << 8) | 0)
#define COLOR_RED(x) (((uint32_t)x << 16) | ((uint32_t)0 << 8) | 0)
#define COLOR_BLUE(x) (((uint32_t)0 << 16) | ((uint32_t)0 << 8) | x)

#define BPM_TO_MS(x) ((uint16_t)((60.0f / ((float)x)) * 1000))

static const float heartbeat_base = 0.15;
static const float heartbeat_peak1 = 0.80;
static const float heartbeat_peak2 = 0.55;
static const float heartbeat_deviation = 0.05;

static LED led(LED_MODE_DEFAULT);

void led_init(void) {
  led.init();
}
void led_exe(void) {
  led.exe();
}
led_color led_get_color() {
  return led.color;
}

void LED::exe(void) {
  static bool test_all_colors = true;
  // Don't run too often
  if (!timer.elapsed()) {
    return;
  }

  switch (state) {
    default:
    case LED_NORMAL:
      // Update brightness
      switch (mode) {
        case led_mode::FLOW:
          flow_run();
          break;
        case led_mode::HEARTBEAT:
          heartbeat_run();
          break;
        case led_mode::CLASSIC:
        default:
          classic_run();
          break;
      }

      // Set color
      switch (get_event_level()) {
        case EVENT_LEVEL_INFO:
          color = led_color::GREEN;
          pixels.setPixelColor(0, COLOR_GREEN(brightness));  // Green pulsing LED
          break;
        case EVENT_LEVEL_WARNING:
          color = led_color::YELLOW;
          pixels.setPixelColor(0, COLOR_YELLOW(brightness));  // Yellow pulsing LED
          break;
        case EVENT_LEVEL_DEBUG:
        case EVENT_LEVEL_UPDATE:
          color = led_color::BLUE;
          pixels.setPixelColor(0, COLOR_BLUE(brightness));  // Blue pulsing LED
          break;
        case EVENT_LEVEL_ERROR:
          color = led_color::RED;
          pixels.setPixelColor(0, COLOR_RED(LED_MAX_BRIGHTNESS));  // Red LED full brightness
          break;
        default:
          break;
      }
      break;
    case LED_COMMAND:
      break;
    case LED_RGB:
      rainbow_run();
      break;
  }

  pixels.show();  // This sends the updated pixel color to the hardware.
}

void LED::classic_run(void) {
  // Determine how bright the LED should be
  brightness = up_down(0.5);
}

void LED::flow_run(void) {
  // Determine how bright the LED should be
  bool power_positive;
  int16_t power_W = datalayer.battery.status.active_power_W;
  if (power_W < -50) {
    // Discharging
    brightness = max_brightness - up_down(0.95);
  } else if (power_W > 50) {
    // Charging
    brightness = up_down(0.95);
  } else {
    brightness = up_down(0.5);
  }
}

void LED::heartbeat_run(void) {
  uint16_t period;
  switch (get_event_level()) {
    case EVENT_LEVEL_WARNING:
      // phew, starting to sweat here
      period = BPM_TO_MS(70);
      break;
    case EVENT_LEVEL_ERROR:
      // omg omg OMG OMGG
      period = BPM_TO_MS(100);
      break;
    default:
      // Keep default chill bpm (ba-dunk... ba-dunk... ba-dunk...)
      period = BPM_TO_MS(35);
      break;
  }

  uint16_t ms = (uint16_t)(millis() % period);

  float period_pct = ((float)ms) / period;
  float brightness_f;

  if (period_pct < 0.10) {
    brightness_f = map_float(period_pct, 0.00f, 0.10f, heartbeat_base, heartbeat_base - heartbeat_deviation);
  } else if (period_pct < 0.20) {
    brightness_f = map_float(period_pct, 0.10f, 0.20f, heartbeat_base - heartbeat_deviation,
                             heartbeat_base - heartbeat_deviation * 2);
  } else if (period_pct < 0.25) {
    brightness_f = map_float(period_pct, 0.20f, 0.25f, heartbeat_base - heartbeat_deviation * 2, heartbeat_peak1);
  } else if (period_pct < 0.30) {
    brightness_f = map_float(period_pct, 0.25f, 0.30f, heartbeat_peak1, heartbeat_base - heartbeat_deviation);
  } else if (period_pct < 0.40) {
    brightness_f = map_float(period_pct, 0.30f, 0.40f, heartbeat_base - heartbeat_deviation, heartbeat_peak2);
  } else if (period_pct < 0.55) {
    brightness_f = map_float(period_pct, 0.40f, 0.55f, heartbeat_peak2, heartbeat_base + heartbeat_deviation * 2);
  } else {
    brightness_f = map_float(period_pct, 0.55f, 1.00f, heartbeat_base + heartbeat_deviation * 2, heartbeat_base);
  }

  brightness = (uint8_t)(brightness_f * LED_MAX_BRIGHTNESS);
}

void LED::rainbow_run(void) {
  brightness = LED_MAX_BRIGHTNESS / 2;

  uint16_t ms = (uint16_t)(millis() % LED_PERIOD_MS);
  float value = ((float)ms) / LED_PERIOD_MS;

  // Clamp the input value to the range [0.0, 1.0]
  value = value < 0.0f ? 0.0f : value;
  value = value > 1.0f ? 1.0f : value;

  uint8_t r = 0, g = 0, b = 0;

  // Scale the value to the range [0, 3), which will be used to transition through the colors
  float scaledValue = value * 3.0f;

  if (scaledValue < 1.0f) {
    // From red to green
    r = static_cast<uint8_t>((1.0f - scaledValue) * brightness);
    g = static_cast<uint8_t>((scaledValue - 0.0f) * brightness);
  } else if (scaledValue < 2.0f) {
    // From green to blue
    g = static_cast<uint8_t>((2.0f - scaledValue) * brightness);
    b = static_cast<uint8_t>((scaledValue - 1.0f) * brightness);
  } else {
    // From blue back to red
    b = static_cast<uint8_t>((3.0f - scaledValue) * brightness);
    r = static_cast<uint8_t>((scaledValue - 2.0f) * brightness);
  }

  // Assemble the color
  uint32_t color = (static_cast<uint32_t>(r) << 16) | (static_cast<uint32_t>(g) << 8) | b;
  pixels.setPixelColor(0, pixels.Color(r, g, b));  // RGB
}

uint8_t LED::up_down(float middle_point_f) {
  // Determine how bright the LED should be
  middle_point_f = CONSTRAIN(middle_point_f, 0.0f, 1.0f);
  uint16_t middle_point = (uint16_t)(middle_point_f * LED_PERIOD_MS);
  uint16_t ms = (uint16_t)(millis() % LED_PERIOD_MS);
  brightness = map_uint16(ms, 0, middle_point, 0, max_brightness);
  if (ms < middle_point) {
    brightness = map_uint16(ms, 0, middle_point, 0, max_brightness);
  } else {
    brightness = LED_MAX_BRIGHTNESS - map_uint16(ms, middle_point, LED_PERIOD_MS, 0, max_brightness);
  }
  return CONSTRAIN(brightness, 0, max_brightness);
}
