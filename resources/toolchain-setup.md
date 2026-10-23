# Toolchain setup

How to open, build and run the `maniq` compiler project, in CLion and from the command line.

## 1. Open in CLion

File → Open → select the repository folder. CLion detects `CMakeLists.txt` and loads the CMake project. If it doesn't, right-click `CMakeLists.txt` → *Load CMake Project*.

CLion help: https://www.jetbrains.com/help/clion/quick-cmake-tutorial.html

## 2. Toolchain

Settings → Build, Execution, Deployment → Toolchains. The bundled **MinGW** toolchain should be the default; it shows the bundled CMake, build tool (make/Ninja) and GCC. The *Debug* CMake profile is fine.

CLion help: https://www.jetbrains.com/help/clion/how-to-create-toolchain-in-clion.html

## 3. Build & run

Select the `maniq` run configuration → Run. Expected output:

```
maniq 0.1.0
```

## 4. Command line (optional)

1. Install CMake from https://cmake.org/download/ — *Windows x64 Installer*, latest stable 4.x. Choose "Add CMake to the system PATH".
2. `cmake` alone isn't enough: a compiler and a build tool must be on PATH too. Add CLion's bundled `<CLion install>\bin\mingw\bin` to PATH; it provides `g++` and `mingw32-make`.
3. Open a new terminal and check: `cmake --version` and `g++ --version`.
4. From the repository root:

   ```
   cmake -S . -B build -G "MinGW Makefiles"
   cmake --build build
   build\maniq.exe
   ```

## 5. Why not the downloaded CMake source

`C:\cmake-4.5.0-rc1` is CMake's *source code*, not a usable CMake. On Windows, building it needs an existing CMake (the `bootstrap` script is a Unix shell script), and it is a release candidate. Optional exercise once CMake is installed: `cmake -S C:\cmake-4.5.0-rc1 -B C:\cmake-build`, then `cmake --build C:\cmake-build`. Not needed for Maniq.

## 6. C++26 note

The project requires C++26, which compilers implement only partially. If a feature fails to compile, check whether GCC supports it yet: https://en.cppreference.com/w/cpp/compiler_support
