cmake_minimum_required(VERSION 3.10)

# Set the C++ standard to C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(BatteryEmulator)

# add_subdirectory(Software/src/devboard/utils)
add_subdirectory(test)
