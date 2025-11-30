### Contributing to the Battery-Emulator project

What can I do? 🦸
--------------

**"Help - I want to contribute something, but I don't know what?"**

You're in luck. There's various sources to contribute:
 - Improve the [Wiki documentation](https://github.com/dalathegreat/Battery-Emulator/wiki)
   - Especially battery/inverter specific pages need updating. Attach pictures of batteries, wiring diagrams, helpful info etc. 
 - Have a look at the [issue tracker](https://github.com/dalathegreat/Battery-Emulator/issues), especially issues with labels:
   - [good first issue](https://github.com/dalathegreat/Battery-Emulator/issues?q=is%3Aissue+is%3Aopen+label%3A%22good+first+issue%22)!
 - Use your favorite text editor to find `TODO` comments in the code
 - Ask us!
   - [Discussion page](https://github.com/dalathegreat/Battery-Emulator/discussions)
   - [Discord server](https://www.patreon.com/dala) 

## Notes on embedded system 🕙
The Battery-Emulator is a real-time control system, which performs lots of time critical operations. Some operations, like contactor control, need to complete within 10 milliseconds periodically. The resources of the ESP32 microcontroller is limited, so keeping track of CPU and memory usage is essential. Keep this in mind when coding for the system! Performance profiling the system can be done by enabling the "Enable performance profiling:" option in the webserver

## Setting up the compilation environment (VScode + PlatformIO) 💻

This project uses the PlatformIO extension within Visual Studio Code for development and uploading. It handles all the complex toolchains and library management for you.
### 1. Installing VSCode
- Download the stable build of Visual Studio Code for your operating system (Windows, macOS, or Linux) from the official website: https://code.visualstudio.com/
- Run the installer and follow the setup instructions.
- (Recommended) Launch VSCode after installation.

### 2. Installing the PlatformIO IDE Plugin

PlatformIO is an extension that adds all the necessary functionality to VSCode.
   - Inside VSCode, open the Extensions view by:
      - Clicking the Extensions icon in the Activity Bar on the left side.
      - Or using the keyboard shortcut: Ctrl+Shift+X (Windows/Linux) or Cmd+Shift+X (macOS).
   - In the extensions search bar, type: PlatformIO IDE.
   - Find the extension published by PlatformIO and click the Install button.
- Wait for the installation to complete. This may take a few minutes as PlatformIO downloads and installs its core tools in the background. VSCode might require a reload once finished.

### 3. Opening the Project and Building
- Clone the repository to your local machine using Git.
- In VSCode:
   - Go to File > Open Folder...
   - Navigate to and select the root folder of the cloned project (the folder containing the platformio.ini file).
   - Click Open.
- PlatformIO will automatically recognize the project structure and begin indexing the code. You'll see the PlatformIO icon (a grey alien) appear in the Activity Bar on the left.

- To verify everything is set up correctly, build/compile the project:
   - Click the PlatformIO icon in the Activity Bar to open the PIO Home screen.
   - Go to Quick Access > PIO > Build.
      - Alternatively, you can use the checkmark icon in the blue status bar at the bottom of the VSCode window, or the keyboard shortcut Ctrl+Alt+B (Windows/Linux) / Cmd+Alt+B (macOS).
   - The build process will start. You can monitor the output in the integrated terminal. A successful build will end with ===== [SUCCESS] Took X.XX seconds =====.

### 4. Uploading Code to Board via USB

- Connect your Battery-Emulator hardware to your computer using a USB cable.
- Select the right board type (Stark, LilyGo)
- At the bottom left of VScode, click the Env to bring up a menu of boards. Select the board you are using
<img width="396" height="95" alt="image" src="https://github.com/user-attachments/assets/23b73442-5016-4ff1-be78-13e3c41772d5" />
<img width="679" height="177" alt="image" src="https://github.com/user-attachments/assets/2fad40a6-f388-4cd6-8e08-844602bb0442" />

- Ensure the correct upload port is set in the platformio.ini file (it's often auto-detected, but you may need to set it manually. See Troubleshooting below).
- Upload the code:
   - Click the PlatformIO icon in the Activity Bar.
   - Go to Quick Access > PIO > Upload.
   - Alternatively, use the right-arrow icon (→) in the blue status bar at the bottom of the VSCode window, or the keyboard shortcut Ctrl+Alt+U (Windows/Linux) / Cmd+Alt+U (macOS).
- The upload process will begin. The board may reset automatically. A successful upload will end with ===== [SUCCESS] Took X.XX seconds =====.

### ⚠️ Troubleshooting & Tips

#### "Upload port not found" or wrong port errors:

- Find the correct port:
    - Windows: Check Device Manager under "Ports (COM & LPT)". It's usually COM3, COM4, etc.
    - macOS/Linux: Run ls /dev/tty.* or ls /dev/ttyUSB* in a terminal. It's often /dev/tty.usbserial-XXX or /dev/ttyUSB0.
- Add the line upload_port = COM4 (replace COM4 with your port) to your platformio.ini file in the [env:...] section.

## Code formatting 📜
The project enforces a specific code formatting in the workflows. To get your code formatted properly, it is easiest to use a pre-commit hook before pushing the code to a pull request.

Before you begin, make sure you have installed Python on the system!
To install the pre-commit, open the repository via Git Bash/CMD, and run
```
pip install pre-commit
```
And then run 
```
pre-commit install
```
Then you can run the autoformat any time with the command
```
pre-commit
```
Or force it to check all files with
```
pre-commit run --all-files
```

## Local Unit test run 🧪
The Unit tests run gtest. Here is how to install this on Debian/Ubuntu and run it locally
```
sudo apt-get install libgtest-dev
sudo apt-get install cmake
```
Navigate to Battery-Emulator/test folder
```
sudo cmake CMakeLists.txt
sudo make
```

## Downloading a pull request build to test locally 🛜
If you want to help test a new feature that is only available in an open pull request, you can download the precompiled binaries from the build system. To do this,start by clicking on the "**Checks**" tab

<img width="779" height="312" alt="image" src="https://github.com/user-attachments/assets/fc7783c1-ba61-440e-ab09-b53d2b49f1bb" />

Then select which hardware you need the binaries for. Currently we build for these hardwares:
- LilyGo T-CAN485
- Stark CMR
- LilyGo T-2CAN

<img width="647" height="476" alt="image" src="https://github.com/user-attachments/assets/fbb97719-0155-4792-9d91-c51e6052fa57" />

After selecting the hardware you need, click the "**Upload Artifact**", and there will be a download link. Download the file, and [OTA Update](https://github.com/dalathegreat/Battery-Emulator/wiki/OTA-Update) your device with this file!

<img width="1714" height="697" alt="image" src="https://github.com/user-attachments/assets/2e17f90f-cc7d-4265-b7bc-7aa5cc6b6ec8" />

