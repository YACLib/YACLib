# Install

## Prerequisites

### Linux

Install GCC/Clang, CMake, make/ninja/etc.

### macOS

Install xcode, CMake.

### Windows

Install MSVC.

## As library

### CMake with FetchContent

```cmake
include(FetchContent)
FetchContent_Declare(yaclib
  GIT_REPOSITORY https://github.com/YACLib/YACLib.git
  GIT_TAG main
  )
FetchContent_MakeAvailable(yaclib)
```

## As project for develop

### Console install and CMake/build options

1. `git clone <repo-url>`
2. In the build directory:

```bash
cmake <source-dir> <cmake-options>
```

CMake options:

* `-G <generator-name>`
  ([more info](https://cmake.org/cmake/help/latest/manual/cmake.1.html#options)).
* `-D CMAKE_BUILD_TYPE=<build-type>`
  ([more info](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)).
* `-D CMAKE_CXX_COMPILER=<cxx-compiler-path>`
  Path to your C++ compiler.
* `-D YACLIB_BUILD_TEST=<OFF(default) or ON or SINGLE>`
  If ON, then build tests, SINGLE, then make one test target
* `-D YACLIB_FLAFS=<EMPTY(default) or WARN or ASAN or TSAN or UBSAN or MEMSAN or COVERAGE>`
  If ON, then.

```bash
cmake --build . -- <build-options>
```

#### Example

In POSIX compliant shell:

```bash
git clone git@github.com:YACLib/YACLib.git
cd YACLib
cmake -S . -B ./build -GNinja -DYACLIB_TEST=ON
cmake --build ./build - --parallel
```
