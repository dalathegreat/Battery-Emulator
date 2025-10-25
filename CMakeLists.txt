cmake_minimum_required(VERSION 3.16)

# IDF's build system
include($ENV{IDF_PATH}/tools/cmake/project.cmake)

# Optional: project version
set(PROJECT_VER "0.1.0")

# Define the app (IDF will look for a "main" component)
project(BatteryEmulator)
