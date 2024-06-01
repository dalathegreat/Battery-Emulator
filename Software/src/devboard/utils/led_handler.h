#ifndef LED_H_
#define LED_H_

#include "../../include.h"
#include "../../lib/adafruit-Adafruit_NeoPixel/Adafruit_NeoPixel.h"
#include "timer.h"

enum led_mode { CLASSIC, FLOW, HEARTBEAT };
enum led_state { LED_NORMAL, LED_COMMAND, LED_RGB };

class LED {
 public:
  led_color color = led_color::GREEN;

  LED()
      : pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800),
        max_brightness(LED_MAX_BRIGHTNESS),
        brightness(LED_MAX_BRIGHTNESS),
        mode(led_mode::CLASSIC),
        state(LED_NORMAL),
        timer(LED_EXECUTION_FREQUENCY) {}

  LED(led_mode mode)
      : pixels(1, LED_PIN, NEO_GRB + NEO_KHZ800),
        max_brightness(LED_MAX_BRIGHTNESS),
        brightness(LED_MAX_BRIGHTNESS),
        mode(mode),
        state(LED_NORMAL),
        timer(LED_EXECUTION_FREQUENCY) {}

  void exe(void);
  void init(void) { pixels.begin(); }

 private:
  Adafruit_NeoPixel pixels;
  uint8_t max_brightness;
  uint8_t brightness;
  led_mode mode;
  led_state state = LED_NORMAL;
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

#endif  // LED_H_
