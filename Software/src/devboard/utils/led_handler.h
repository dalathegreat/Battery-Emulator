#ifndef LED_H_
#define LED_H_

#include "../../devboard/utils/types.h"
#include "../../lib/adafruit-Adafruit_NeoPixel/Adafruit_NeoPixel.h"

static const uint32_t LED_COLOR_WHITE = 0xFFFFFF;  // R=G=B=255

class LED {
 public:
  LED(gpio_num_t pin, uint8_t maxBrightness)
      : pixels(pin), max_brightness(maxBrightness), brightness(maxBrightness), mode(led_mode_enum::CLASSIC) {}

  LED(led_mode_enum mode, gpio_num_t pin, uint8_t maxBrightness)
      : pixels(pin), max_brightness(maxBrightness), brightness(maxBrightness), mode(mode) {}

  void exe(void);

 private:
  Adafruit_NeoPixel pixels;
  uint8_t max_brightness;
  uint8_t brightness;
  led_mode_enum mode;

  void classic_run(void);
  void flow_run(void);
  void heartbeat_run(void);

  uint8_t up_down(uint16_t middle_point_f);
  uint16_t LED_PERIOD_MS = 3000;
};

bool led_init(void);
void led_exe(void);

// Temporarily override the LED for button-hold feedback: blinks `color` on/off at
// `period_ms`. Pass active=false to release and resume normal battery-state behavior.
// No-op when no LED is present (GPIO unset, or configured as an OLED instead).
void set_led_override(bool active, uint32_t color, uint16_t period_ms);

#endif  // LED_H_
