# [YACLib](https://github.com/YACLib/YACLib)

[![GitHub license](
https://img.shields.io/badge/license-MIT-blue.svg)](
https://raw.githubusercontent.com/YACLib/YACLib/main/LICENSE)
[![FOSSA status](
https://app.fossa.com/api/projects/git%2Bgithub.com%2FYACLib%2FYACLib.svg?type=shield)](
https://app.fossa.com/projects/git%2Bgithub.com%2FYACLib%2FYACLib)

[![Test](
https://github.com/YACLib/YACLib/actions/workflows/test.yml/badge.svg?branch=main)](
https://github.com/YACLib/YACLib/actions/workflows/test.yml)
[![Test with Google sanitizer](
https://github.com/YACLib/YACLib/actions/workflows/google_sanitizer.yml/badge.svg?branch=main)](
https://github.com/YACLib/YACLib/actions/workflows/google_sanitizer.yml)
[![Check code format](
https://github.com/YACLib/YACLib/actions/workflows/code_format.yml/badge.svg?branch=main)](
https://github.com/YACLib/YACLib/actions/workflows/code_format.yml)

[![Test coverage](
https://codecov.io/gh/YACLib/YACLib/branch/main/graph/badge.svg)](
https://codecov.io/gh/YACLib/YACLib)
[![Codacy Badge](
https://app.codacy.com/project/badge/Grade/4113686840a645a8950abdf1197611bd)](
https://www.codacy.com/gh/YACLib/YACLib/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=YACLib/YACLib&amp;utm_campaign=Badge_Grade)

YACLib (_Yet Another Concurrency Library_) is a C++ library for concurrent tasks execution.

## Table of Contents
* [About YACLib](#about)
* [Getting started](#quickstart)
* Support
* Contributing
* License
* Useful links

<a name="about"></a>
## About YACLib
**YACLib** is a lightweight C++ library for concurrent and parallel task execution, that is striving to satisfy the following properties:
* Easy to use
* Easy to build
* Zero cost abstractions
* Good test coverage

## Getting started
For quick start just paste this code in your CMakeLists.txt
```cmake
include(FetchContent)
FetchContent_Declare(yaclib
        GIT_REPOSITORY https://github.com/YACLib/YACLib.git
        GIT_TAG main
        )
FetchContent_MakeAvailable(yaclib)
link_libraries(yaclib)
```
## Useful links

### For user

* [Documentation](https://yaclib.github.io/YACLib)
* [Examples](test/example)
* [Install guide](doc/install.md)
* [Benchmarks](TODO(MBkkt))

### For developer

* [Specification](https://yaclib.github.io/YACLib)
* [Targets description](doc/target.md)
* [About dependencies](doc/dependency.md)
* [Pull request guide](doc/pr_guide.md)
* [Style guide](doc/style_guide.md)

## License

[![FOSSA Status](
https://app.fossa.com/api/projects/git%2Bgithub.com%2FYACLib%2FYACLib.svg?type=large)](
https://app.fossa.com/projects/git%2Bgithub.com%2FYACLib%2FYACLib?ref=badge_large)
