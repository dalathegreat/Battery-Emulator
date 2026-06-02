# CYD Battery Emulator Screen

This is a separate screen project for `Battery-Emulator`.

It runs on a CYD touchscreen board and receives Battery Emulator data over ESP-NOW.

The goal is simple: show battery data on a small screen and allow a few basic control actions.

## What it can do

- show battery data on the screen
- work with one battery
- work with two batteries
- show cell voltages
- show fault messages
- show battery and inverter status
- connect to Wi-Fi
- support OTA update from the web page

It can also send a few control commands back to the emulator:

- pause
- resume
- reboot emulator
- open contactors
- close contactors

## How the screens work

If only one battery is active, it shows the single battery screen.

If two batteries are active, it shows the dual battery screen.

The screen changes automatically based on the data it receives.

There are also extra screens for:

- battery 1 cells
- battery 2 cells
- faults
- secret/control page

## Secret / control screen

The control screen can:

- pause or resume the emulator
- open or close contactors
- reboot the emulator

Each action has a confirmation popup so it is harder to press something by mistake.

## Web page

The screen creates its own Wi-Fi access point so it can always be configured.

Current AP name:

- `BatteryEmulator-CYD`

The web page can be used for:

- entering Wi-Fi name and password
- setting static IP if needed
- forgetting saved Wi-Fi
- restarting the screen
- uploading new firmware with OTA

## Screen behavior

- after boot, it opens the correct main screen
- after 3 minutes without touch, brightness goes down to 30%
- after 5 minutes without touch, the screen turns off
- touching the screen wakes it back up

## Build

This is a PlatformIO project.

Main environment:

- `cyd`

Example:

```powershell
pio run -e cyd
```

## Main files

- `src/main.cpp` - starts the screen, touch and app
- `src/espnow_receiver.cpp` - receives ESP-NOW data and updates the UI
- `src/screen_network.cpp` - Wi-Fi, web page and OTA
- `src/screen_backlight.cpp` - dimming and wake-up behavior
- `src/ui/` - generated UI files

## Important note

This is still a testing version.

It has not been fully tested on a real system yet.

It should be treated as development / bench software until more testing is done.
