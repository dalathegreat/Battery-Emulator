# Battery-Emulator ⚡🔋
![GitHub release (with filter)](https://img.shields.io/github/v/release/dalathegreat/BYD-Battery-Emulator-For-Gen24?color=%23008000)
![GitHub Repo stars](https://img.shields.io/github/stars/dalathegreat/Battery-Emulator?style=flat&color=%23128512)
![GitHub forks](https://img.shields.io/github/forks/dalathegreat/Battery-Emulator?style=flat&color=%23128512)
![GitHub actions](https://img.shields.io/github/actions/workflow/status/dalathegreat/BYD-Battery-Emulator-For-Gen24/compile-all-batteries.yml?color=0E810E)
![Static Badge](https://img.shields.io/badge/made-with_love-blue?color=%23008000)

This software enables EV battery packs to be used for stationary storage. It achieves this by converting the EV battery CAN data into a brand battery format that solar inverters can understand. This makes it extremely cheap and easy to use large EV batteries in a true plug'n'play fashion!

> [!CAUTION]
> Working with high voltage is dangerous. Always follow local laws and regulations regarding high voltage work. If you are unsure about the rules in your country, consult a licensed electrician for more information.

![Fronius](https://github.com/dalathegreat/Battery-Emulator/assets/26695010/741c3237-8074-4891-9cd1-f47f0fe45cb5)


## Hardware requirements 📜
This code fits on the LilyGo ESP32 T-CAN485 devboard , see https://github.com/Xinyuan-LilyGO/T-CAN485

You will also need a complete EV battery. [See the battery compatibility list on which are supported.](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki#supported-batteries-list)

Finally, you will need a [compatible hybrid solar inverter](https://github.com/dalathegreat/BYD-Battery-Emulator-For-Gen24/wiki#supported-inverters-list), for example the "Fronius Gen24" or "GoodWe ET"

## Installation basics 🪛
1. Connect one end of the LilyGo RS485 to the Gen24 Modbus
2. Connect the other end of the LilyGo to the CAN side of the battery
3. Wire up high voltage cable between the Gen24 and the battery
4. Add a 5-12V power source to power the LilyGo and 12V to the battery (uninterruptible PSU or 12V lead acid recommended in parallel)
5. Some batteries need manual pre-charge circuit and positive/negative contactor control. Others are automatic. See the wiki for more info.
6. Enjoy a big cheap grid connected battery!

## Wiring example, LEAF battery 💡
Here's how to wire up the communication between the components.
![Wiring](https://github.com/dalathegreat/Battery-Emulator/assets/26695010/29edeeda-1002-4826-9183-39a027b3b9ed)


Here's how to connect the high voltage lines
![HighVoltageWiring](https://github.com/dalathegreat/Battery-Emulator/assets/26695010/f70e6262-d630-4148-9a39-dad32e79b3d6)

For more examples showing wiring, see each battery types own Wiki page. For instance the [Nissan LEAF page](https://github.com/dalathegreat/Battery-Emulator/wiki/Battery:-Nissan-LEAF---e%E2%80%90NV200)

## How to compile the software 💻

Start by watching this [quickstart guide](https://www.youtube.com/watch?v=hcl2GdHc0Y0)

[![IMAGE ALT TEXT HERE](https://img.youtube.com/vi/hcl2GdHc0Y0/0.jpg)](https://www.youtube.com/watch?v=hcl2GdHc0Y0)

1. Download the Arduino IDE: https://www.arduino.cc/en/software
2. Open the Arduino IDE.
3. Click `File` menu -> `Preferences` -> `Additional Development` -> `Additional Board Manager URLs` -> Enter the URL in the input box: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` and click OK.
4. Click `Tools` menu -> `Board: "...."` -> `Boards Manager...`, install the `esp32` package by `Espressif Systems` (not `Arduino ESP32 Boards`), then press `Close`.

**NOTE: The ESP32 version depends on which release of Battery-Emulator you are running!**

- ⚠️ Make sure to use a 2.x.x version if you are on a release **older** than 6.0.0 (For instance ESP32 v2.0.11 when using Battery-Emulator v5.4.0)
- ⚠️ Make sure to use a 3.0.x version if you are on a release **newer** than 6.0.0 (For instance ESP32 v3.0.0 when using Battery-Emulator v6.0.0)
- ⚠️ Make sure to use a 3.1.x version if you are on a release **newer** than 8.0.0 (For instance ESP32 v3.1.0 when using Battery-Emulator v8.0.0)
  
![bild](https://github.com/dalathegreat/Battery-Emulator/assets/26695010/6a2414b1-f2ca-4746-8e8d-9afd78bd9252)

5. The Arduino board should be set to `ESP32 Dev Module` and `Partition Scheme` to `Minimal SPIFFS (1.9MB APP with OTA/190KB SPIFFS)` (under `Tools` -> `Board` -> `ESP32 Arduino`) with the following settings:

![ArduinoSettings](https://github.com/user-attachments/assets/74d36b07-cca4-4bf1-9eaf-1e7fa4e1effe)

6. Select which battery type you will use, along with other optional settings. This is done in the `USER_SETTINGS.h` file.
7. Copy the `USER_SECRETS.TEMPLATE.h` file to `USER_SECRETS.h` and update connectivity settings inside this file.
8. Press `Verify` and `Upload` to send the sketch to the board.
NOTE: In some cases, the LilyGo must be powered through the main power connector instead of USB-C
      when performing the initial firmware upload.
NOTE: On Mac, the following USB driver may need to be installed: https://github.com/WCHSoftGroup/ch34xser_macos

NOTE: If you see garbled messages on the serial console, change the serial console to match the baud rate to the code, currently 115200.

NOTE: If you see the error `Sketch too big` then check you set the Partition Scheme above correctly.

This video explains all the above mentioned steps:
<https://youtu.be/_mH2AjnAjDk>


### Linux Development Environment Setup
In addition to the steps above, ESP32 requires a dependency for a Python module, pyserial install using the cli.\
```python3 -m pip install pyserial```

If you're using Ubuntu , use apt to manage the dependencies of arduino:\
pyserial install: ```sudo apt install python3-serial```

Arduino AppImage must be set as executable after downloading to run correctly\
example: ```chmod 775 arduino-ide_2.3.3_Linux_64bit.AppImage```

## Dependencies 📖
This code uses the following excellent libraries: 
- [adafruit/Adafruit_NeoPixel](https://github.com/adafruit/Adafruit_NeoPixel) LGPL-3.0 license
- [ayushsharma82/ElegantOTA](https://github.com/ayushsharma82/ElegantOTA) AGPL-3.0 license 
- [bblanchon/ArduinoJson](https://github.com/bblanchon/ArduinoJson) MIT-License
- [eModbus/eModbus](https://github.com/eModbus/eModbus) MIT-License
- [ESP32Async/AsyncTCP](https://github.com/ESP32Async/AsyncTCP) LGPL-3.0 license
- [ESP32Async/ESPAsyncWebServer](https://github.com/ESP32Async/ESPAsyncWebServer) LGPL-3.0 license
- [miwagner/ESP32-Arduino-CAN](https://github.com/miwagner/ESP32-Arduino-CAN/) MIT-License
- [pierremolinaro/acan2515](https://github.com/pierremolinaro/acan2515) MIT-License
- [pierremolinaro/acan2517FD](https://github.com/pierremolinaro/acan2517FD) MIT-License
- [YiannisBourkelis/Uptime-Library](https://github.com/YiannisBourkelis/Uptime-Library) GPL-3.0 license 

It is also based on the information found in the following excellent repositories/websites:
- https://gitlab.com/pelle8/inverter_resources //new url
- https://github.com/burra/byd_battery
- https://github.com/flodorn/TeslaBMSV2
- https://github.com/SunshadeCorp/can-service
- https://github.com/openvehicles/Open-Vehicle-Monitoring-System-3
- https://github.com/dalathegreat/leaf_can_bus_messages
- https://github.com/rand12345/solax_can_bus
- https://github.com/Tom-evnut/BMWI3BMS/ SMA-CAN
- https://github.com/FozzieUK/FoxESS-Canbus-Protocol FoxESS-CAN
- https://github.com/maciek16c/hyundai-santa-fe-phev-battery
- https://github.com/ljames28/Renault-Zoe-PH2-ZE50-Canbus-LBC-Information
- Renault Zoe CAN Matrix https://docs.google.com/spreadsheets/u/0/d/1Qnk-yzzcPiMArO-QDzO4a8ptAS2Sa4HhVu441zBzlpM/edit?pli=1#gid=0
- Pylon hacking https://www.eevblog.com/forum/programming/pylontech-sc0500-protocol-hacking/

## Like this project? 💖
Leave a ⭐ If you think this project is useful. Consider hopping onto my Patreon to encourage more open-source projects!

<a href="https://www.patreon.com/dala">
	<img src="https://c5.patreon.com/external/logo/become_a_patron_button@2x.png" width="160">
</a>
