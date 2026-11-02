#include "led_handler.h"
#include "../../datalayer/battery_aggregate.h"
#include "../../datalayer/datalayer.h"
#include "../../devboard/hal/hal.h"
#include "events.h"
#include "value_mapping.h"

#define COLOR_GREEN(x) (((uint32_t)0 << 16) | ((uint32_t)x << 8) | 0)
#define COLOR_YELLOW(x) (((uint32_t)x << 16) | ((uint32_t)x << 8) | 0)
#define COLOR_RED(x) (((uint32_t)x << 16) | ((uint32_t)0 << 8) | 0)
#define COLOR_BLUE(x) (((uint32_t)0 << 16) | ((uint32_t)0 << 8) | x)

// Scales a full-brightness 24-bit color's R/G/B channels by `brightness` (0-255).
static uint32_t scale_color(uint32_t color, uint8_t brightness) {
  uint8_t r = (uint8_t)(color >> 16), g = (uint8_t)(color >> 8), b = (uint8_t)color;
  r = (uint16_t)r * brightness / 255;
  g = (uint16_t)g * brightness / 255;
  b = (uint16_t)b * brightness / 255;
  return ((uint32_t)r << 16) | ((uint32_t)g << 8) | b;
}

#define BPM_TO_MS(x) ((60000 / (x)))  // 60 * 1000 = 60000

#define HEARTBEAT_BASE 150      // 0.15 * 1000
#define HEARTBEAT_PEAK1 800     // 0.80 * 1000
#define HEARTBEAT_PEAK2 550     // 0.55 * 1000
#define HEARTBEAT_DEVIATION 50  // 0.05 * 1000
#define SCALE_FACTOR 1000

uint8_t user_selected_led_pin = 255;
uint8_t user_selected_led_count = 0;

/* Energy Flow counts anything within this many Watts of zero as idle. Some BMS current sensors
   bounce by as much as 0.5 A around zero with no current flowing, which reads as up to ~200 W on
   a high voltage pack and would otherwise make the pattern flicker between its three states. */
#define FLOW_IDLE_BAND_W 200
/* Energy Flow's charge/discharge ramp: 2.91 s one way, 0.09 s the other, of the 3 s period */
#define FLOW_RAMP_SCALED 970  // 0.97 * 1000

static LED* led = nullptr;

static bool led_override_active = false;
static uint32_t led_override_color = 0;
static uint16_t led_override_period_ms = 0;

static bool indicator_led_state[4] = {false, false, false, false};

void set_led_override(bool active, uint32_t color, uint16_t period_ms) {
  if (led == nullptr) {
    return;  // no LED (GPIO unset or used for an OLED) — nothing to drive
  }
  led_override_active = active;
  led_override_color = color;
  led_override_period_ms = period_ms;
}

void set_indicator_led(IndicatorLed indicator, bool on) {
  indicator_led_state[static_cast<uint8_t>(indicator)] = on;
}

uint16_t map_int(uint16_t x, uint16_t in_min, uint16_t in_max, uint16_t out_min, uint16_t out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

bool led_init(void) {
  // A user-set GPIO overrides the board default. GPIO_NUM_MAX bounds it; alloc_pins() below
  // still rejects a pin another peripheral already owns.
  gpio_num_t led_pin = (user_selected_led_pin < GPIO_NUM_MAX) ? (gpio_num_t)user_selected_led_pin
                                                              : esp32hal->LED_PIN();

  if (led_pin == GPIO_NUM_NC) {
    return true;
  }

  if (!esp32hal->alloc_pins("LED", led_pin)) {
    DEBUG_PRINTF("LED setup failed\n");
    return false;
  }

  // A user-set strip length overrides the board default, so one build suits any strip.
  const uint8_t led_count = user_selected_led_count > 0 ? user_selected_led_count : esp32hal->LED_COUNT();

  led = new LED(datalayer.battery.status.led_mode, led_pin, esp32hal->LED_MAX_BRIGHTNESS(), led_count);

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

  // SOC bar owns the whole strip: length shows charge level, colour shows what the pack is
  // doing. It writes every pixel itself, so it skips the status/indicator handling below.
  if (mode == led_mode_enum::SOC_BAR) {
    soc_bar_run();
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
  uint32_t status_hue = 0;  // full-brightness hue, reused below for the indicator LEDs
  switch (get_emulator_status()) {
    case EMULATOR_STATUS::STATUS_OK:
      status_hue = COLOR_GREEN(0xFF);
      pixels.setPixelColor(COLOR_GREEN(brightness));  // Green pulsing LED
      break;
    case EMULATOR_STATUS::STATUS_WARNING:
      status_hue = COLOR_YELLOW(0xFF);
      pixels.setPixelColor(COLOR_YELLOW(brightness));  // Yellow pulsing LED
      break;
    case EMULATOR_STATUS::STATUS_ERROR:
      status_hue = COLOR_RED(0xFF);
      pixels.setPixelColor(COLOR_RED(esp32hal->LED_MAX_BRIGHTNESS()));  // Red LED full brightness
      break;
    case EMULATOR_STATUS::STATUS_UPDATING:
      status_hue = COLOR_BLUE(0xFF);
      pixels.setPixelColor(COLOR_BLUE(brightness));  // Blue pulsing LED
      break;
  }

  // Indicator LEDs 1-4 mirror the STATUS LED's color, at a fixed (non-animated) brightness.
  // Skipped entirely on boards with no RGB indicator LEDs (num_leds == 1), rather than relying
  // on Adafruit_NeoPixel::setPixelColor()'s bounds check to make it a no-op.
  if (num_leds > 1) {
    uint32_t indicator_color = scale_color(status_hue, max_brightness);
    for (uint8_t i = 0; i < 4; i++) {
      pixels.setPixelColor(i + 1, indicator_led_state[i] ? indicator_color : 0);
    }
  }

  pixels.show();  // This sends the updated pixel color to the hardware.
}

void LED::soc_bar_run(void) {
  // Colour says what the pack is doing; bar length says how full it is.
  uint32_t hue = LED_COLOR_WHITE;
  bool blink = false;

  switch (get_emulator_status()) {
    case EMULATOR_STATUS::STATUS_ERROR:
      hue = COLOR_RED(0xFF);
      blink = true;
      break;
    case EMULATOR_STATUS::STATUS_WARNING:
      hue = COLOR_YELLOW(0xFF);
      break;
    case EMULATOR_STATUS::STATUS_UPDATING:
      hue = COLOR_BLUE(0xFF);
      blink = true;
      break;
    default:
      if (datalayer.battery.status.active_power_W > 50) {
        hue = COLOR_GREEN(0xFF);  // charging
      } else if (datalayer.battery.status.active_power_W < -50) {
        hue = COLOR_BLUE(0xFF);  // discharging
      }  // otherwise idle, leave it white
      break;
  }

  // Faults and firmware updates blink the whole bar, so they read as "look at me" rather than
  // as a charge level.
  if (blink && ((millis() / 500) % 2) == 0) {
    for (uint8_t i = 0; i < num_leds; i++) {
      pixels.setPixelColor(i, 0);
    }
    return;
  }

  // Each pixel is one 1/num_leds slice of the pack. The pixel the level currently sits in is
  // dimmed in proportion to how far into its slice the SOC has got, giving sub-slice resolution
  // instead of the bar jumping a whole pixel at a time.
  const uint32_t scaled = (uint32_t)datalayer.battery.status.reported_soc * num_leds;  // soc is 0-10000
  const uint8_t filled = (uint8_t)(scaled / 10000);
  const uint32_t partial = scaled % 10000;

  for (uint8_t i = 0; i < num_leds; i++) {
    uint8_t level;
    if (i < filled) {
      level = max_brightness;
    } else if (i == filled) {
      level = (uint8_t)((uint32_t)max_brightness * partial / 10000);
      if (level < 2) {
        level = 2;  // keep the first pixel faintly lit at 0 %, so the strip never looks dead
      }
    } else {
      level = 0;
    }
    pixels.setPixelColor(i, level > 0 ? scale_color(hue, level) : 0);
  }
}

void LED::classic_run(void) {
  // Determine how bright the LED should be
  brightness = up_down(500);
}

void LED::flow_run(void) {
  // Determine how bright the LED should be
  // Pack 1's power when it is the whole installation, the aggregate's with double/triple
  const int32_t power_W = (datalayer.system.info.configured_batteries > 1) ? datalayer.aggregate.active_power_W
                                                                           : pack_power_W(datalayer.battery.status);
  if (power_W < -FLOW_IDLE_BAND_W) {
    // Discharging
    brightness = max_brightness - up_down(FLOW_RAMP_SCALED);
  } else if (power_W > FLOW_IDLE_BAND_W) {
    // Charging
    brightness = up_down(FLOW_RAMP_SCALED);
  } else {  // Idle
    brightness = up_down(500);
  }
}

void LED::heartbeat_run(void) {
  // No case for errors: exe() shows them as steady red, so no beat would ever be seen
  const uint16_t period = (get_event_level() == EVENT_LEVEL_WARNING) ? BPM_TO_MS(70) : BPM_TO_MS(35);

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
