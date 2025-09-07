#include "led_handler.h"
#include "../../datalayer/datalayer.h"
#include "../../devboard/hal/hal.h"
#include "events.h"
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

static LED* led;

bool led_init(void) {
  if (!esp32hal->alloc_pins("LED", esp32hal->LED_PIN())) {
    DEBUG_PRINTF("LED setup failed\n");
    return false;
  }

  led = new LED(datalayer.battery.status.led_mode, esp32hal->LED_PIN(), esp32hal->LED_MAX_BRIGHTNESS());

  return true;
}

void led_exe(void) {
  led->exe();
}

void LED::exe(void) {

  // Update brightness
  switch (datalayer.battery.status.led_mode) {
    case led_mode_enum::FLOW:
      flow_run();
      break;
    case led_mode_enum::HEARTBEAT:
      heartbeat_run();
      break;
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
  brightness = up_down(0.5);
}

void LED::flow_run(void) {
  // Determine how bright the LED should be
  if (datalayer.battery.status.active_power_W < -50) {
    // Discharging
    brightness = max_brightness - up_down(0.95);
  } else if (datalayer.battery.status.active_power_W > 50) {
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

  brightness = (uint8_t)(brightness_f * esp32hal->LED_MAX_BRIGHTNESS());
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
    brightness = esp32hal->LED_MAX_BRIGHTNESS() - map_uint16(ms, middle_point, LED_PERIOD_MS, 0, max_brightness);
  }
  return CONSTRAIN(brightness, 0, max_brightness);
}
