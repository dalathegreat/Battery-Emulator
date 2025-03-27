#ifndef LED_H_
#define LED_H_

#include "../../include.h"
#include "../../lib/adafruit-Adafruit_NeoPixel/Adafruit_NeoPixel.h"

class LED {
 public:
  led_color color = led_color::GREEN;

  LED()
      : pixels(1, LED_PIN, NEO_GRB),
        max_brightness(LED_MAX_BRIGHTNESS),
        brightness(LED_MAX_BRIGHTNESS),
        mode(led_mode_enum::CLASSIC) {}

  LED(led_mode_enum mode)
      : pixels(1, LED_PIN, NEO_GRB), max_brightness(LED_MAX_BRIGHTNESS), brightness(LED_MAX_BRIGHTNESS), mode(mode) {}

  void exe(void);
  void init(void) { pixels.begin(); }

 private:
  Adafruit_NeoPixel pixels;
  uint8_t max_brightness;
  uint8_t brightness;
  led_mode_enum mode;

  void classic_run(void);
  void flow_run(void);
  void heartbeat_run(void);

  uint8_t up_down(float middle_point_f);
};

void led_init(void);
void led_exe(void);
led_color led_get_color(void);

#endif  // LED_H_
