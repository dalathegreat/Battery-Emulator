mkdir build
cd build
cmake .. -G "Visual Studio 17 2022" -T ClangCL -A x64
cmake --build .
