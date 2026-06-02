// pauLTU3
// CYD backlight runtime behavior.
// Testing-only build.
// This logic has not been validated on a real system and should be treated as experimental.

#include "screen_backlight.h"

#include <Arduino.h>

namespace {

// Brightness timeouts in milliseconds.
constexpr uint32_t DIM_AFTER_MS = 180000U;
constexpr uint32_t OFF_AFTER_MS = 300000U;

// Logical brightness levels in percent.
constexpr uint8_t FULL_BRIGHTNESS = 100U;
constexpr uint8_t DIM_BRIGHTNESS = 30U;
constexpr uint8_t OFF_BRIGHTNESS = 0U;

// Timestamp of the last known touch event.
uint32_t g_last_touch_ms = 0U;

// Last brightness value written by this module.
// This is used to avoid writing the same PWM value over and over.
uint8_t g_last_brightness = 255U;

// Convert a simple 0-100 percent brightness value to 0-255 PWM range.
uint8_t percent_to_pwm(uint8_t percent) {
    if (percent >= 100U) {
        return 255U;
    }
    return static_cast<uint8_t>((static_cast<uint16_t>(percent) * 255U) / 100U);
}

// Write the new backlight level only when it actually changes.
void apply_backlight_percent(uint8_t percent) {
    if (percent == g_last_brightness) {
        return;
    }

    g_last_brightness = percent;

    // Some boards define ON as HIGH, others as LOW.
    // The platformio.ini macros decide which direction to use.
#if TFT_BACKLIGHT_ON == HIGH
    analogWrite(TFT_BL, percent_to_pwm(percent));
#else
    analogWrite(TFT_BL, 255U - percent_to_pwm(percent));
#endif
}

}  // namespace

void screen_backlight_init() {
    // Prepare the physical backlight pin.
    pinMode(TFT_BL, OUTPUT);

    // Start the inactivity timer now so the screen stays bright after boot.
    g_last_touch_ms = millis();

    // Force first write on init.
    g_last_brightness = 255U;

    // Boot at full brightness.
    apply_backlight_percent(FULL_BRIGHTNESS);
}

void screen_backlight_note_touch() {
    // Any touch means the user is active again.
    g_last_touch_ms = millis();

    // Wake immediately instead of waiting for the periodic update tick.
    apply_backlight_percent(FULL_BRIGHTNESS);
}

void screen_backlight_update() {
    // How long the screen has been untouched.
    const uint32_t idle_ms = millis() - g_last_touch_ms;

    // After one minute fully switch off the backlight.
    if (idle_ms >= OFF_AFTER_MS) {
        apply_backlight_percent(OFF_BRIGHTNESS);
        return;
    }

    // After thirty seconds dim to half brightness.
    if (idle_ms >= DIM_AFTER_MS) {
        apply_backlight_percent(DIM_BRIGHTNESS);
        return;
    }

    // Otherwise keep it fully on.
    apply_backlight_percent(FULL_BRIGHTNESS);
}
