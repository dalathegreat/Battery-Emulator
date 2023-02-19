# BYD-Battery-Emulator-For-Gen24
This software converts the LEAF CAN into Modbus RTU registers understood by solar inverters that take the BYD 11kWh HVM battery

## Hardware requirements
This code runs on the LilyGo ESP32 T-CAN485 devboard , see https://github.com/Xinyuan-LilyGO/T-CAN485

## How to compile the software
1. Download the Arduino IDE: https://www.arduino.cc/en/software
2. When the Arduino IDE has been started;
Click "File" in the upper left corner -> Preferences -> Additional Development >Board Manager URL -> Enter the URL in the input box https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
3. Go to "Boards Manager", and install the ESP32 package by Espressif Systems
4. The arduino settings should be set to "ESP32 Dev Module" with the following settings;

5. Press Verify and Upload to send the sketch to the board

