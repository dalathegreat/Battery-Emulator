// pauLTU3
// CYD backlight control interface.
// Testing-only build.
// This behavior is intended for development use and has not been validated on a real system.

// - full brightness while the screen is used,
// - dim after 180 seconds,
// - turn fully off after 300 seconds,
// - wake instantly on touch.

#ifndef SCREEN_BACKLIGHT_H
#define SCREEN_BACKLIGHT_H

// Configure the backlight output pin and start in fully on state.
void screen_backlight_init();

// Run the idle timer logic and apply the right brightness level.
void screen_backlight_update();

// Tell the module that the user touched the screen.
void screen_backlight_note_touch();

#endif
