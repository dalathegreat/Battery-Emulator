# BYD-Battery-Emulator-For-Gen24
This software converts the LEAF CAN into Modbus RTU registers understood by solar inverters that accept the BYD 11kWh HVM battery

## Hardware requirements
This code fits on the LilyGo ESP32 T-CAN485 devboard , see https://github.com/Xinyuan-LilyGO/T-CAN485
You will also need a Nissan LEAF battery, any model year will do
Finally, you will need a hybrid solar inverter that accepts the BYD battery communication standard, for example the Fronius Gen24

## Installation basics
1. Connect one end of the LilyGo RS485 to the Gen24 Modbus
2. Connect the other end of the LilyGo to the CAN side of a LEAF battery
3. Wire up high voltage cable between the Gen24 and the LEAF battery
4. Add a 12V power source to power the LilyGo and the LEAF battery (uninterruptible PSU or 12V lead acid recommended in parallel)
5. Manually handle pre-charge circuit + positive/negative contactor on LEAF battery for now (circuit will be improved soon)
6. Enjoy a big cheap grid connected battery!

## Wiring examples
Here's how to wire up the communication between the components.
![alt text](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/blob/main/Images/Wiring.png)

Here's how to connect the high voltage lines
![alt text](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/blob/main/Images/HighVoltageWiring.png)

Here's how to wire up battery low voltage wiring
![alt text](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/blob/main/Images/BatteryControlWiring.png)

## How to compile the software
1. Download the Arduino IDE: https://www.arduino.cc/en/software
2. When the Arduino IDE has been started;
Click "File" in the upper left corner -> Preferences -> Additional Development >Board Manager URL -> Enter the URL in the input box https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
3. Go to "Boards Manager", and install the ESP32 package by Espressif Systems
4. The arduino settings should be set to "ESP32 Dev Module" with the following settings;
![alt text](https://github.com/Xinyuan-LilyGO/T-CAN485/blob/main/img/arduino_setting.png)
5. Press Verify and Upload to send the sketch to the board

## Dependencies
This code uses two libraries, ESP32-Arduino-CAN (https://github.com/miwagner/ESP32-Arduino-CAN/) slightly modified for this usecase, and the eModbus library (https://github.com/eModbus/eModbus). Both these are already located in the Software folder
