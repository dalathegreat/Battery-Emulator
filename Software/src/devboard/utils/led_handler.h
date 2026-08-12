#ifndef LED_H_
#define LED_H_

#include <soc/gpio_num.h>
#include "../../devboard/utils/types.h"
#include "../../lib/adafruit-Adafruit_NeoPixel/Adafruit_NeoPixel.h"

static const uint32_t LED_COLOR_WHITE = 0xFFFFFF;  // R=G=B=255

// Indicator LEDs 1-4 in the chain, for boards that can replace their hardwired 12V-output LEDs
// with RGB LEDs (pixel 0 is always the STATUS LED and is unaffected by this).
enum class IndicatorLed : uint8_t { PRECHARGE = 0, CONTACTOR_NEG = 1, CONTACTOR_POS = 2, BMS_POWER = 3 };

class LED {
 public:
  LED(gpio_num_t pin, uint8_t maxBrightness, uint8_t numLeds = 1)
      : pixels(pin, numLeds),
        max_brightness(maxBrightness),
        brightness(maxBrightness),
        mode(led_mode_enum::CLASSIC),
        num_leds(numLeds) {}

  LED(led_mode_enum mode, gpio_num_t pin, uint8_t maxBrightness, uint8_t numLeds = 1)
      : pixels(pin, numLeds), max_brightness(maxBrightness), brightness(maxBrightness), mode(mode), num_leds(numLeds) {}

  void exe(void);

 private:
  Adafruit_NeoPixel pixels;
  uint8_t max_brightness;
  uint8_t brightness;
  led_mode_enum mode;
  uint8_t num_leds;  // 1 = STATUS only; >1 = STATUS + RGB indicator LEDs are present

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

// Mirrors on/off state onto an RGB indicator LED, for boards where an indicator position is an
// RGB LED instead of a plain hardwired GPIO LED. No-op on boards without RGB indicator LEDs.
void set_indicator_led(IndicatorLed indicator, bool on);

#endif  // LED_H_
