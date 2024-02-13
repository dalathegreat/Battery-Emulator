@echo off
echo Cleaning up
rmdir /Q/S build
echo Creating new CMake build folder
mkdir build
cd build
echo Building CMake project
call cmake ..
call cmake --build .
echo Executing tests
for %%i in ("test\Debug\*.exe") do (
    echo Running %%i
    %%i
)
cd..
