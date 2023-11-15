# Battery Emulator Webserver
This webserver creates a WiFi access point. It also connects ot an existing network.
The webserver intends to display useful information to the user of the battery emulator
development board, without the need to physically connect to the board via USB.
The webserver implementation also provides the option to update the firmware of the board over the air.

To use the webserver, follow the following steps:
- Connect to the board via Serial, and boot up the board.
- The IP address of the WiFi access point is printed to Serial when the board boots up. Note this down.
- Connect to the access point created by board via a WiFi-capable device
- On that device, open a webbrowser and type the IP address of the WiFi access point.
- If the ssid and password of an existing WiFi network are provided, the board will also connect to this network. The IP address obtained on the existing network is shown in the webserver. Note this down.
- From this point onwards, any device connected to the existing WiFi network can access the webserver via a webbrowser. To do this:
  - Connect your WiFi-capable device to the existing nwetork.
  - Open a webbrowser and type the IP address obtained on the existing WiFi network.

To update the software over the air:
- In Arduino, go to `Sketch` > `Export Compiled Binary`. This will create the `.bin` file that you need to update the firmware. It is found in the folder `Software/build/`
- In your webbrowser, go to the url consisting of the IP address, followed by `/update`, for instance `http://192.168.0.224/update`.
- In the webbrowser, follow the steps to select the `.bin` file and to upload the file to the board.

## Future work
This section lists a number of features that can be implemented as part of the webserver in the future.

- TODO: display state of charge
- TODO: display battery hardware selected
- TODO: display emulated inverter communication protocol selected
- TODO: list all available ssids: scan WiFi Networks https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/
- TODO: add option to add/change ssid and password and save, connect to the new ssid (Option: save ssid and password using Preferences.h library https://randomnerdtutorials.com/esp32-save-data-permanently-preferences/)
- TODO: display WiFi connection strength (https://randomnerdtutorials.com/esp32-useful-wi-fi-functions-arduino/)
- TODO: display CAN state (indicate if there is a communication error)
- TODO: display battery errors in battery diagnosis tab
- TODO: display inverter errors in battery diagnosis tab
- TODO: add functionality to turn WiFi AP off
- TODO: fix IP address on home network (https://randomnerdtutorials.com/esp32-static-fixed-ip-address-arduino-ide/)
- TODO: set hostname (https://randomnerdtutorials.com/esp32-set-custom-hostname-arduino/)

# References
The code for this webserver is based on code provided by Rui Santos at https://randomnerdtutorials.com.
