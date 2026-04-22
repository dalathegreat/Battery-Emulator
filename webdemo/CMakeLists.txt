cmake_minimum_required(VERSION 3.28)

# set the project name
project(BatteryEmulator LANGUAGES CXX C)

# Define FreeRTOS configuration before FetchContent
add_library(freertos_config INTERFACE)
target_include_directories(freertos_config SYSTEM INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/freertos_config")
target_compile_definitions(freertos_config INTERFACE projCOVERAGE_TEST=0)

include(FetchContent)

FetchContent_Declare(
    FreeRTOS-Kernel
    URL      https://github.com/FreeRTOS/FreeRTOS-Kernel/archive/refs/tags/V11.1.0.tar.gz
)

# Set port before include
set(FREERTOS_PORT "ThirdParty/GCC/Posix" CACHE STRING "")
set(FREERTOS_COMPILER "GCC" CACHE STRING "")

# If the automated port selection is still failing, we can manually add the source
FetchContent_GetProperties(FreeRTOS-Kernel)
if(NOT freertos-kernel_POPULATED)
    FetchContent_Populate(FreeRTOS-Kernel)
    add_subdirectory(${freertos-kernel_SOURCE_DIR} ${freertos-kernel_BINARY_DIR} EXCLUDE_FROM_ALL)
endif()

# Manually add the POSIX port sources if the kernel's portable/CMakeLists.txt missed them
target_sources(freertos_kernel_port PRIVATE
    ${freertos-kernel_SOURCE_DIR}/portable/ThirdParty/GCC/Posix/port.c
    ${freertos-kernel_SOURCE_DIR}/portable/ThirdParty/GCC/Posix/utils/wait_for_event.c
    ${freertos-kernel_SOURCE_DIR}/portable/MemMang/heap_4.c
)
target_include_directories(freertos_kernel_port PUBLIC
    ${freertos-kernel_SOURCE_DIR}/portable/ThirdParty/GCC/Posix
)

# Ensure pthread for POSIX port
find_package(Threads REQUIRED)

# specify the C++ standard
# At least 17 is required by Gtest
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

include_directories(emul ../test/emul ../Software)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NO_POSIX_ERROR_CODES ESP32 ARDUINO ASYNCTCP_ALLOW_ANY_ERRNOS_VALUES HW_PC DEBUG_VIA_USB COMMON_IMAGE ARDUINOJSON_ENABLE_PROGMEM=0 CONFIG_IDF_TARGET_ESP32 HW_LILYGO UNIT_TEST inverter_sends_000=0)

set_source_files_properties(../Software/Software.cpp PROPERTIES LANGUAGE CXX)

#file(GLOB BATTERY_SOURCE "../Software/src/battery/*.cpp")
#file(GLOB INVERTER_SOURCE "../Software/src/inverter/*.cpp")
#file(GLOB CHARGER_SOURCE "../Software/src/charger/*.cpp")

# add the executable
add_executable(BatteryEmulator 
    webdemo_main.cpp 
    can_mocks.cpp
    vars.cpp
    ../Software/src/lib/TinyWebServer/CanSender.cpp
    ../Software/src/lib/TinyWebServer/Ota.cpp
    ../Software/src/lib/TinyWebServer/TinyWebServer.cpp
    ../Software/src/lib/TinyWebServer/webserver2.cpp
    ../Software/src/lib/TinyWebServer/webserver_api.cpp
    ../Software/src/lib/TinyWebServer/webserver_can_logging.cpp
    ../Software/src/lib/TinyWebServer/webserver_settings.cpp
    ../Software/src/lib/TinyWebServer/webserver_utils.cpp
    ../Software/src/core/parallel_safety.cpp
    ../Software/src/communication/can/obd.cpp
    ../Software/src/communication/contactorcontrol/comm_contactorcontrol.cpp
    ../Software/src/communication/nvm/comm_nvm.cpp
    ../Software/src/communication/precharge_control/precharge_control.cpp
    ../Software/src/communication/rs485/comm_rs485.cpp
    ../Software/src/devboard/safety/safety.cpp
    ../Software/src/devboard/hal/hal.cpp
    ../Software/src/devboard/utils/types.cpp
    ../Software/src/devboard/utils/events.cpp
    ../Software/src/devboard/utils/common_functions.cpp
    ../Software/src/devboard/webserver/webserver.cpp
    ../Software/src/devboard/wifi/wifi.cpp
    ../Software/src/datalayer/datalayer.cpp
    ../Software/src/datalayer/datalayer_extended.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusMessage.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusServer.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusServerRTU.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusTypeDefs.cpp
    ../Software/src/lib/eModbus-eModbus/RTUutils.cpp
    ../Software/src/battery/BATTERIES.cpp
    ../Software/src/battery/Battery.cpp
    ../Software/src/battery/BMW-I3-BATTERY.cpp
    ../Software/src/battery/BMW-I3-HTML.cpp
    ../Software/src/battery/BMW-IX-BATTERY.cpp
    ../Software/src/battery/BMW-IX-HTML.cpp
    ../Software/src/battery/BMW-PHEV-BATTERY.cpp
    ../Software/src/battery/BMW-SBOX.cpp
    ../Software/src/battery/BOLT-AMPERA-BATTERY.cpp
    ../Software/src/battery/BYD-ATTO-3-BATTERY.cpp
    ../Software/src/battery/CanBattery.cpp
    ../Software/src/battery/CELLPOWER-BMS.cpp
    ../Software/src/battery/CHADEMO-BATTERY.cpp
    ../Software/src/battery/CHADEMO-CT.cpp
    ../Software/src/battery/CHADEMO-SHUNTS.cpp
    ../Software/src/battery/CMFA-EV-BATTERY.cpp
    ../Software/src/battery/CMP-SMART-CAR-BATTERY.cpp
    ../Software/src/battery/DALY-BMS.cpp
    ../Software/src/battery/ECMP-BATTERY.cpp
    ../Software/src/battery/ENNOID-BMS.cpp
    ../Software/src/battery/FORD-MACH-E-BATTERY.cpp
    ../Software/src/battery/FOXESS-BATTERY.cpp
    ../Software/src/battery/GEELY-GEOMETRY-C-BATTERY.cpp
    ../Software/src/battery/GROWATT-HV-ARK-BATTERY.cpp
    ../Software/src/battery/HYUNDAI-IONIQ-28-BATTERY-HTML.cpp
    ../Software/src/battery/HYUNDAI-IONIQ-28-BATTERY.cpp
    ../Software/src/battery/IMIEV-CZERO-ION-BATTERY.cpp
    ../Software/src/battery/JAGUAR-IPACE-BATTERY.cpp
    ../Software/src/battery/KIA-E-GMP-BATTERY.cpp
    ../Software/src/battery/KIA-E-GMP-HTML.cpp
    ../Software/src/battery/KIA-64FD-BATTERY.cpp
    ../Software/src/battery/KIA-HYUNDAI-64-BATTERY.cpp
    ../Software/src/battery/KIA-HYUNDAI-HYBRID-BATTERY.cpp
    ../Software/src/battery/MEB-BATTERY.cpp
    ../Software/src/battery/MG-5-BATTERY.cpp
    ../Software/src/battery/MG-HS-PHEV-BATTERY.cpp
    ../Software/src/battery/NISSAN-LEAF-BATTERY.cpp
    ../Software/src/battery/ORION-BMS.cpp
    ../Software/src/battery/PYLON-BATTERY.cpp
    ../Software/src/battery/RANGE-ROVER-PHEV-BATTERY.cpp
    ../Software/src/battery/RELION-LV-BATTERY.cpp
    ../Software/src/battery/RENAULT-KANGOO-BATTERY.cpp
    ../Software/src/battery/RENAULT-TWIZY.cpp
    ../Software/src/battery/RENAULT-ZOE-GEN1-BATTERY.cpp
    ../Software/src/battery/RENAULT-ZOE-GEN2-BATTERY.cpp
    ../Software/src/battery/RIVIAN-BATTERY.cpp
    ../Software/src/battery/RJXZS-BMS.cpp
    ../Software/src/battery/SAMSUNG-SDI-LV-BATTERY.cpp
    ../Software/src/battery/SANTA-FE-PHEV-BATTERY.cpp
    ../Software/src/battery/Shunts.cpp
    ../Software/src/battery/SIMPBMS-BATTERY.cpp
    ../Software/src/battery/SONO-BATTERY.cpp
    ../Software/src/battery/TESLA-BATTERY.cpp
    ../Software/src/battery/TESLA-LEGACY-BATTERY.cpp
    ../Software/src/battery/TEST-FAKE-BATTERY.cpp
    ../Software/src/battery/THINK-BATTERY.cpp
    ../Software/src/battery/THUNDERSTRUCK-BMS.cpp
    ../Software/src/battery/GEELY-SEA-BATTERY.cpp
    ../Software/src/battery/VOLVO-SPA-BATTERY.cpp
    ../Software/src/battery/VOLVO-SPA-HYBRID-BATTERY.cpp
    ../Software/src/inverter/AFORE-CAN.cpp
    ../Software/src/inverter/BYD-CAN.cpp
    ../Software/src/inverter/BYD-MODBUS.cpp
    ../Software/src/inverter/FERROAMP-CAN.cpp
    ../Software/src/inverter/FOXESS-CAN.cpp
    ../Software/src/inverter/GROWATT-HV-CAN.cpp
    ../Software/src/inverter/GROWATT-LV-CAN.cpp
    ../Software/src/inverter/GROWATT-WIT-CAN.cpp
    ../Software/src/inverter/INVERTERS.cpp
    ../Software/src/inverter/KOSTAL-RS485.cpp
    ../Software/src/inverter/ModbusInverterProtocol.cpp
    ../Software/src/inverter/PYLON-CAN.cpp
    ../Software/src/inverter/PYLON-LV-RS485.cpp
    ../Software/src/inverter/PYLON-LV-CAN.cpp
    ../Software/src/inverter/SCHNEIDER-CAN.cpp
    ../Software/src/inverter/SMA-BYD-H-CAN.cpp
    ../Software/src/inverter/SMA-BYD-HVS-CAN.cpp
    ../Software/src/inverter/SMA-SBS-BYD-CAN.cpp
    ../Software/src/inverter/SMA-LV-CAN.cpp
    ../Software/src/inverter/SOFAR-CAN.cpp
    ../Software/src/inverter/SOL-ARK-LV-CAN.cpp
    ../Software/src/inverter/SOLAX-CAN.cpp
    ../Software/src/inverter/SOLXPOW-CAN.cpp
    ../Software/src/inverter/SUNGROW-CAN.cpp
    ../Software/src/inverter/VCU-CAN.cpp
    ../Software/src/charger/CHARGERS.cpp
    ../Software/src/charger/CHEVY-VOLT-CHARGER.cpp
    ../Software/src/charger/NISSAN-LEAF-CHARGER.cpp
    ../test/emul/can.cpp
    ../test/emul/time.cpp
    ../test/emul/serial.cpp
    ../test/emul/Arduino.cpp
    ../test/emul/Preferences.cpp
    )

target_link_libraries(BatteryEmulator PRIVATE freertos_kernel freertos_kernel_port Threads::Threads)

# Ensure we have the include path for <freertos/FreeRTOS.h>
# The FreeRTOS Kernel uses "FreeRTOS.h", but ESP32 codebases often use "freertos/FreeRTOS.h"
# We'll create a header shim or use include paths.
# Let's add the freertos-kernel source directory to the include path so that its 'include' 
# directory can be accessed as 'include/...' (less useful)
# Best approach: create a 'freertos' directory in our config area and put shims in it.

# The POSIX port needs to be able to find its headers
target_include_directories(BatteryEmulator PRIVATE "${freertos-kernel_SOURCE_DIR}/portable/ThirdParty/GCC/Posix")

    # ${BATTERY_SOURCE}
    # ${CHARGER_SOURCE}
    # ${INVERTER_SOURCE}
