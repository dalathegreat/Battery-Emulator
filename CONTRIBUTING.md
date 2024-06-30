### Contributing to the Battery-Emulator project

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
