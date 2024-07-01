### Contributing to the Battery-Emulator project

## Notes on embedded system
The Battery-Emulator is a real-time control system, which performs lots of time critical operations. Some operations, like contactor control, need to complete within 10 milliseconds periodically. The resources of the ESP32 microcontroller is limited, so keeping track of CPU and memory usage is essential. Keep this in mind when coding for the system! Performance profiling the system can be done by enabling the FUNCTION_TIME_MEASUREMENT option in the USER_SETTINGS.h file

## Code formatting
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
