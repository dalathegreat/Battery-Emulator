cmake_minimum_required(VERSION 3.28)
enable_testing()

# set the project name
project(UnitTests)

# Enable ExternalProject CMake module
include(ExternalProject)
include(GoogleTest)

# specify the C++ standard
# At least 17 is required by Gtest
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Download and install GoogleTest
ExternalProject_Add(
    gtest
    URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/gtest
    # Disable install step
    INSTALL_COMMAND ""
)

# Get GTest source and binary directories from CMake project
ExternalProject_Get_Property(gtest source_dir binary_dir)

# Create a libgtest target to be used as a dependency by test programs
add_library(libgtest IMPORTED STATIC GLOBAL)
add_dependencies(libgtest gtest)

# Set libgtest properties
if(WIN32)
    set_target_properties(libgtest PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/lib/Debug/gtest.lib"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")
else()
    set_target_properties(libgtest PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/lib/libgtest.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")
endif(WIN32)

# Create a libgmock target to be used as a dependency by test programs
add_library(libgmock IMPORTED STATIC GLOBAL)
add_dependencies(libgmock gtest)

# Set libgmock properties
if(WIN32)
    set_target_properties(libgmock PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/lib/Debug/gmock.lib"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")
else()
    set_target_properties(libgmock PROPERTIES
        "IMPORTED_LOCATION" "${binary_dir}/lib/libgmock.a"
        "IMPORTED_LINK_INTERFACE_LIBRARIES" "${CMAKE_THREAD_LIBS_INIT}")
endif(WIN32)

# I couldn't make it work with INTERFACE_INCLUDE_DIRECTORIES
include_directories("${source_dir}/googletest/include"
                    "${source_dir}/googlemock/include")

include_directories(emul)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NO_POSIX_ERROR_CODES ESP32 ARDUINO HW_LILYGO NISSAN_LEAF_BATTERY)

set_source_files_properties(../Software/Software.ino PROPERTIES LANGUAGE CXX)

# add the executable
add_executable(tests 
    tests.cpp 
    #main_tests.cpp 
    safety_tests.cpp 
    battery/NissanLeafTest.cpp 
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
#    ../Software/src/lib/eModbus-eModbus/scripts/mbServerFCs.cpp
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
    emul/can.cpp
    emul/time.cpp
    emul/serial.cpp
    emul/WString.cpp
    emul/cbuf.cpp
    emul/WiFi.cpp
    emul/Arduino.cpp
    emul/Update.cpp
    emul/MD5Builder.cpp
    emul/HardwareSerial.cpp
    emul/Preferences.cpp
    emul/print.cpp
    emul/mqtt.cpp
    emul/freertos/task.cpp
    emul/freertos/semphr.cpp
    emul/freertos/ringbuf.cpp
    emul/lwip/sockets.cpp
    emul/lwip/dns.cpp
    emul/libb64/cencode.cpp
    )

target_link_libraries(tests
    libgtest
    libgmock
)

gtest_discover_tests(tests)
