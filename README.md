# CSEngine
A C++ game engine.

# How to Build
This project is optimized to be built on Windows using MSVC.

1. Ensure that you have [MSVC](https://visualstudio.microsoft.com/downloads/) installed.
2. Ensure that you have [CMake](https://cmake.org/download/) installed, you can run `winget install Kitware.CMake` if
   you don't.
3. Ensure that you have [LLVM](https://releases.llvm.org/) installed, you can run `winget install LLVM.LLVM` and put the
   install location in your environment variables if you don't (for language server and clang-format support).
4. Execute `script/build.sh` followed by `script/run.sh`.

# How to Update Dependencies
All dependencies are managed by CPM.cmake. After changing the version of a dependency, do a full clean build using
`script/clean.sh` before `script/build.sh`.

### [CPM.cmake](https://github.com/cpm-cmake/CPM.cmake/releases)
Change the `CPM_VERSION` variable inside `cmake/Manage.cmake` to the desired version.

### [SDL](https://github.com/libsdl-org/SDL/releases) and [SDL_Shadercross](https://github.com/libsdl-org/SDL_shadercross)
Change the `[DEPENDECY]_VERSION` variable inside `cmake/Deps.cmake` to the desired version.
