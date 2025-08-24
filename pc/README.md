# Building Battery Emulator for PC

The emulator can be built as an executable for PC environments for development and testing.

These builds are not officially supported or recommended for any production use.

## Install cmake

Cmake is used to build for both Windows and Linux. You need to have that installed.

## Build for Windows

For Windows builds you also need Visual Studio 2022.

```
cd pc
mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -T ClangCL -A x64
cmake --build .
```

This produces `BatteryEmulator.exe` that you can run.


## Build for Linux

```
cd pc
mkdir buildLinux
cd buildLinux
cmake ..
cmake --build .
```

To run:

```
sudo ./BatteryEmulator
```
