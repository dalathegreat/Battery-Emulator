# CYD screen

The CYD screen is a separate touchscreen display project for `Battery-Emulator`.

It receives Battery Emulator data over ESP-NOW and shows it on a small local screen.

## Battery Emulator context

The goal of the CYD screen is to give a simple local display for Battery Emulator without needing a phone, PC, or inverter web page.

It can be used to:

- view live battery data
- view cell voltages
- view fault messages
- view battery and inverter state
- view one battery or two batteries automatically

It can also send a few control commands back to the emulator:

- pause
- resume
- open contactors
- close contactors
- reboot emulator

## Screen behavior

If only one battery is active, the screen shows the single battery layout.

If two batteries are active, the screen shows the dual battery layout.

The battery cell pages and fault page are updated from the ESP-NOW data sent by Battery Emulator.

The screen also has a control page with confirmation popups before sending commands.

## Web page and OTA

The CYD screen has its own web page.

It can:

- create its own access point
- connect to local Wi-Fi
- save Wi-Fi credentials
- use static IP settings
- upload new firmware with OTA

Current AP name:

- `BatteryEmulator-CYD`

## Power saving

The screen brightness is reduced after inactivity:

- after 3 minutes, brightness goes down to 30%
- after 5 minutes, the screen turns off
- touching the screen wakes it up again

## Project location

The full CYD screen project is here:

- [CYD_Battery_Emulator](./CYD_Battery_Emulator/README.md)

## Important note

This is still a testing version.

It has not been fully tested on a real system yet.
