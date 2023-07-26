# Battery-Emulator ⚡🔋
This software enables EV battery packs to be used for stationary storage in combination with solar inverters. It achieves this by converting EV battery CAN data into Modbus RTU registers, that emulate a BYD 11kWh HVM battery. This makes it extremely easy to use EV batteries in a true plug'n'play fashion.

![alt text](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/blob/main/Images/Fronius.png)

## Hardware requirements 📜
This code fits on the LilyGo ESP32 T-CAN485 devboard , see https://github.com/Xinyuan-LilyGO/T-CAN485

You will also need a complete EV battery. [See the battery compability list on which are supported.](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki#supported-batteries-list)

Finally, you will need a [compatible hybrid solar inverter](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki#supported-inverters-list) that accepts the BYD battery communication standard, for example the Fronius Gen24

## Installation basics 🪛
1. Connect one end of the LilyGo RS485 to the Gen24 Modbus
2. Connect the other end of the LilyGo to the CAN side of the battery
3. Wire up high voltage cable between the Gen24 and the battery
4. Add a 12V power source to power the LilyGo and the battery (uninterruptible PSU or 12V lead acid recommended in parallel)
5. Some batteries need manual pre-charge circuit and positive/negative contactor control. Others are automatic. See the wiki for more info.
6. Enjoy a big cheap grid connected battery!

## Wiring example, LEAF battery 💡
Here's how to wire up the communication between the components.
![alt text](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/blob/main/Images/Wiring.png)

Here's how to connect the high voltage lines
![alt text](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/blob/main/Images/HighVoltageWiring.png)

Here's how to wire up battery low voltage wiring
![alt text](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/blob/main/Images/BatteryControlWiring.png)

For more examples showing wiring, see the Example#####.jpg pictures in the 'Images' folder
https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/tree/main/Images

## How to compile the software 💻
1. Download the Arduino IDE: https://www.arduino.cc/en/software
2. When the Arduino IDE has been started;
Click "File" in the upper left corner -> Preferences -> Additional Development >Board Manager URL -> Enter the URL in the input box https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
3. Go to "Boards Manager", and install the ESP32 package by Espressif Systems
4. The arduino settings should be set to "ESP32 Dev Module" with the following settings;
![alt text](https://github.com/Xinyuan-LilyGO/T-CAN485/blob/main/img/arduino_setting.png)
5. Select which battery type you will use, along with other optional settings
6. Press Verify and Upload to send the sketch to the board.

## Dependencies 📖
This code uses two libraries, ESP32-Arduino-CAN (https://github.com/miwagner/ESP32-Arduino-CAN/) slightly modified for this usecase, and the eModbus library (https://github.com/eModbus/eModbus). Both these are already located in the Software folder for an easy start.

It is also based on the info found in the following excellent repositories/websites:
- https://gitlab.com/pelle8/gen24
- https://github.com/burra/byd_battery
- https://github.com/flodorn/TeslaBMSV2
- https://github.com/SunshadeCorp/can-service
- https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3
- https://github.com/dalathegreat/leaf_can_bus_messages
- https://github.com/rand12345/solax_can_bus
- Renault Zoe CAN Matrix https://docs.google.com/spreadsheets/u/0/d/1Qnk-yzzcPiMArO-QDzO4a8ptAS2Sa4HhVu441zBzlpM/edit?pli=1#gid=0
- Pylon hacking https://www.eevblog.com/forum/programming/pylontech-sc0500-protocol-hacking/

## Like this project? 💖
Leave a ⭐ If you think this project is useful. Consider hopping onto my Patreon to encourage more open-source projects!

<a href="https://www.patreon.com/dala">
	<img src="https://c5.patreon.com/external/logo/become_a_patron_button@2x.png" width="160">
</a>
