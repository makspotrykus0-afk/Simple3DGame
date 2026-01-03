---
description: Build project using CMake and MinGW
---
// turbo-all
1. Prepare build directory: `cmake -B build -G "MinGW Makefiles"`
2. Build project (CMake): `cmake --build build --config Debug`
3. Build project (Make): `mingw32-make -C build`
4. Alternative build: `cd build; cmake --build . --config Debug`
5. Manual build: `mingw32-make`
6. Build and Run: `cd build; mingw32-make; ./Simple3DGame`
