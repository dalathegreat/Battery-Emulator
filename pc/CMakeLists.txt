cmake_minimum_required(VERSION 3.28)

# set the project name
project(BatteryEmulator)

# specify the C++ standard
# At least 17 is required by Gtest
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(../test/emul)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NO_POSIX_ERROR_CODES ESP32 ARDUINO HW_PC NISSAN_LEAF_BATTERY)

set_source_files_properties(../Software/Software.ino PROPERTIES LANGUAGE CXX)

# add the executable
add_executable(BatteryEmulator 
    pc_main.cpp 
    ../Software/src/communication/precharge_control/precharge_control.cpp
    ../Software/src/communication/contactorcontrol/comm_contactorcontrol.cpp
    ../Software/src/communication/equipmentstopbutton/comm_equipmentstopbutton.cpp
    ../Software/src/communication/nvm/comm_nvm.cpp
    ../Software/src/communication/rs485/comm_rs485.cpp
    ../Software/src/devboard/wifi/wifi.cpp
    ../Software/src/devboard/mqtt/mqtt.cpp
    ../Software/src/devboard/safety/safety.cpp
    ../Software/src/devboard/hal/hal.cpp
    ../Software/src/devboard/utils/events.cpp
    ../Software/src/devboard/utils/logging.cpp
    ../Software/src/devboard/utils/debounce_button.cpp
    ../Software/src/datalayer/datalayer.cpp
    ../Software/src/datalayer/datalayer_extended.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusServer.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusServerRTU.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusMessage.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusTypeDefs.cpp
    ../Software/src/lib/eModbus-eModbus/Logging.cpp
    ../Software/src/lib/eModbus-eModbus/scripts/mbServerFCs.cpp
    ../Software/src/lib/eModbus-eModbus/RTUutils.cpp
    ../Software/src/lib/ESP32Async-ESPAsyncWebServer/src/AsyncWebHeader.cpp
    ../Software/src/lib/ESP32Async-ESPAsyncWebServer/src/AsyncWebServerRequest.cpp
    ../Software/src/lib/ESP32Async-ESPAsyncWebServer/src/WebServer.cpp
    ../Software/src/lib/ESP32Async-ESPAsyncWebServer/src/WebRequest.cpp
    ../Software/src/lib/ESP32Async-ESPAsyncWebServer/src/WebHandlers.cpp
    ../Software/src/lib/ESP32Async-ESPAsyncWebServer/src/WebResponses.cpp
    ../Software/src/lib/ESP32Async-ESPAsyncWebServer/src/WebAuthentication.cpp
    ../Software/src/lib/ESP32Async-ESPAsyncWebServer/src/Middleware.cpp
    ../Software/src/lib/mathieucarbou-AsyncTCPSock/src/AsyncTCP.cpp
    ../Software/src/lib/adafruit-Adafruit_NeoPixel/Adafruit_NeoPixel.cpp
    ../Software/src/lib/ayushsharma82-ElegantOTA/src/elop.cpp
    ../Software/src/lib/ayushsharma82-ElegantOTA/src/ElegantOTA.cpp
    ../Software/src/lib/YiannisBourkelis-Uptime-Library/src/uptime_formatter.cpp
    ../Software/src/lib/YiannisBourkelis-Uptime-Library/src/uptime.cpp
    ../Software/USER_SETTINGS.cpp
    ../Software/src/battery/BATTERIES.cpp
    ../Software/src/battery/Battery.cpp
    ../Software/src/battery/CanBattery.cpp
    ../Software/src/battery/NISSAN-LEAF-BATTERY.cpp
    ../Software/src/battery/Shunts.cpp
    ../Software/src/inverter/INVERTERS.cpp
    ../Software/src/inverter/BYD-MODBUS.cpp
    ../Software/src/inverter/ModbusInverterProtocol.cpp
    ../Software/src/charger/CHARGERS.cpp
    ../Software/src/devboard/webserver/advanced_battery_html.cpp
    ../Software/src/devboard/webserver/can_logging_html.cpp
    ../Software/src/devboard/webserver/can_replay_html.cpp
    ../Software/src/devboard/webserver/cellmonitor_html.cpp
    ../Software/src/devboard/webserver/debug_logging_html.cpp
    ../Software/src/devboard/webserver/events_html.cpp
    ../Software/src/devboard/webserver/index_html.cpp
    ../Software/src/devboard/webserver/settings_html.cpp
    ../Software/src/devboard/webserver/webserver.cpp
    ../Software/src/devboard/utils/types.cpp
    ../Software/src/devboard/utils/timer.cpp
    ../Software/src/devboard/utils/led_handler.cpp
    ../Software/Software.ino
    ../test/emul/can.cpp
    ../test/emul/cbuf.cpp
    ../test/emul/time.cpp
    ../test/emul/serial.cpp
    ../test/emul/WString.cpp
    ../test/emul/WiFi.cpp
    ../test/emul/Arduino.cpp
    ../test/emul/Update.cpp
    ../test/emul/MD5Builder.cpp
    ../test/emul/HardwareSerial.cpp
    ../test/emul/Preferences.cpp
    ../test/emul/print.cpp
    ../test/emul/mqtt.cpp
    ../test/emul/freertos/task.cpp
    ../test/emul/freertos/semphr.cpp
    ../test/emul/lwip/sockets.cpp
    ../test/emul/lwip/dns.cpp
    ../test/emul/libb64/cencode.cpp
    )
