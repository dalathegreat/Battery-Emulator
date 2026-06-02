// pauLTU3
// CYD ESP-NOW receiver interface.
// Testing-only build.
// This code has not been verified on a real system and must be treated as experimental.

// Public entry points for the ESP-NOW receiver module.
// This module owns:
// - receiving Battery Emulator packets,
// - mapping packet data to LVGL UI objects,
// - keeping the UI alive when packets briefly stop,
// - handling remote control commands from the secret screen.

#ifndef ESPNOW_RECEIVER_H
#define ESPNOW_RECEIVER_H

// Initialize the receiver, register ESP-NOW callbacks and prime the UI.
void espnow_receiver_init();

// Run periodic UI and runtime update logic.
void espnow_receiver_update();

// Force the UI to show the correct main screen after boot.
void espnow_receiver_show_main_screen();

#endif
