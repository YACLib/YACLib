# Install

## Prerequisites

### Linux

Install GCC/Clang, CMake, make/Ninja.

## Console install and CMake/build options

1. `git clone <repo-url>`
2. In the build directory:

```
cmake <source-dir> <cmake-options>
```

CMake options:

* `-G <generator-name>`
  ([more info](https://cmake.org/cmake/help/latest/manual/cmake.1.html#options)).
* `-D CMAKE_BUILD_TYPE=<build-type>`
  ([more info](https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html)).
* `-D CMAKE_CXX_COMPILER=<cxx-compiler-path>`
  Path to your C++ compiler.
* `-D BUILD_TESTING=<OFF(default) or ON>`
  If ON, then build tests.
* `-D ENABLE_LTO=<OFF(default) or ON>`
  If ON, then enable Link-Time Optimization.

```
cmake --build . -- <build-options>
```

Build options:

* `-j <parallel-jobs>`
  The logical number of cores, how many threads the compiler will use.

### Example

In POSIX compliant shell:

```bash
git clone git@github.com:YACLib/YACLib.git
cd YACLib
mkdir build
cd build
cmake .. -G Ninja -D BUILD_TESTING=ON
cmake --build . -- -j 8
```
