#ifndef LED_H_
#define LED_H_

#include "../../include.h"
#include "../../lib/adafruit-Adafruit_NeoPixel/Adafruit_NeoPixel.h"
#include "timer.h"

enum led_mode { CLASSIC, FLOW, HEARTBEAT };
enum led_state { NORMAL, COMMAND, RGB };

class LED {
 public:
  led_color color = led_color::GREEN;

  LED()
      : mode(led_mode::CLASSIC),
        max_brightness(LED_MAX_BRIGHTNESS),
        pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800),
        timer(10) {}
  LED(led_mode mode)
      : mode(mode), max_brightness(LED_MAX_BRIGHTNESS), pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800), timer(10) {}

  void exe(void);
  void init(void) { pixels.begin(); }

 private:
  Adafruit_NeoPixel pixels;
  uint8_t max_brightness;
  uint8_t brightness;
  led_mode mode = led_mode::CLASSIC;
  led_state state = led_state::NORMAL;
  MyTimer timer;

  void classic_run(void);
  void flow_run(void);
  void rainbow_run(void);
  void heartbeat_run(void);

  uint8_t up_down(float middle_point_f);
};

void led_init(void);
void led_exe(void);
led_color led_get_color(void);

#endif
