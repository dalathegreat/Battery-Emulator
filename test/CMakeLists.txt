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

# For eModBus
add_compile_definitions(ESP32 HW_LILYGO NISSAN_LEAF_BATTERY)

set_source_files_properties(../Software/Software.ino PROPERTIES LANGUAGE CXX)

# add the executable
add_executable(tests 
    tests.cpp 
    safety_tests.cpp 
    battery/NissanLeafTest.cpp 
    ../Software/src/devboard/safety/safety.cpp
    ../Software/src/devboard/hal/hal.cpp
    ../Software/src/devboard/utils/events.cpp
    ../Software/src/datalayer/datalayer.cpp
    ../Software/src/datalayer/datalayer_extended.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusMessage.cpp
    ../Software/src/lib/eModbus-eModbus/ModbusTypeDefs.cpp
    ../Software/src/lib/eModbus-eModbus/Logging.cpp
#    ../Software/src/lib/eModbus-eModbus/scripts/mbServerFCs.cpp
    ../Software/USER_SETTINGS.cpp
    ../Software/src/battery/BATTERIES.cpp
    ../Software/src/battery/Battery.cpp
    ../Software/src/battery/CanBattery.cpp
    ../Software/src/battery/NISSAN-LEAF-BATTERY.cpp
    ../Software/src/inverter/INVERTERS.cpp
    ../Software/src/inverter/BYD-MODBUS.cpp
    ../Software/src/charger/CHARGERS.cpp
    emul/can.cpp
    emul/time.cpp
    emul/serial.cpp
    emul/Arduino.cpp
#    ../Software/Software.ino
    )

target_link_libraries(tests
    libgtest
    libgmock
)

gtest_discover_tests(tests)
