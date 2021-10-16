# YACLib
_Yet Another Concurrency Library_

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


## Table of Contents
* [About YACLib](#about)
* [Getting started](#quickstart)
* [Examples](#examples)
* [Benchmarks](TODO(MBkkt))
* [Contributing](#contrib)
* [Support](#support)
* [License](#license)



<a name="about"></a>

## About YACLib
**YACLib** is a lightweight C++ library for concurrent and parallel task execution, that is striving to satisfy the following properties:
* Easy to use
* Easy to build
* Zero cost abstractions
* Good test coverage

For more details check our [design document](doc/design.md) and [documentation](https://yaclib.github.io/YACLib).

<a name="quickstart"></a>

## Getting started
For quick start just paste this code in your `CMakeLists.txt` file
```cmake
include(FetchContent)
FetchContent_Declare(yaclib
  GIT_REPOSITORY https://github.com/YACLib/YACLib.git
  GIT_TAG main
  )
FetchContent_MakeAvailable(yaclib)
link_libraries(yaclib)
```
For more details check [install guide](doc/install.md).

<a name="examples"></a>

## Examples
Here are short examples of using some features from YACLib, for details check [documentation](https://yaclib.github.io/YACLib/examples.html).

<details><summary>Asynchronous pipeline</summary><p>

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
yaclib::Run(tp, [] { return 42; })
  .Then([](int r) { return r * 2; })
  .Then([](int r) { return r + 1; })
  .Then([](int r) { return std::to_string(r); })
  .Subscribe([](std::string r) {
    std::cout << "Pipeline result: <"  << r << ">" << std::endl;
  });
};
```
</p></details>

<details><summary>Exception recovering from callbacks</summary><p>

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
auto f = yaclib::Run(tp, [] { 
    return 1; 
  }).Then([](int y) { 
    throw std::runtime_error{""}; 
  }).Then([](int z) {
    return z * 2; // Will  not run
  }).Then([](std::exception_ptr) {
    return 15; 
  }); //  Recover from exception
int x = std::move(f).Get().Value(); // 15
```
</p></details>

<details><summary>Error recovering from callbacks</summary><p>

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
auto f = yaclib::Run(tp, [] {
    if (random() % 2) {
      return std::make_error_code(1);
    }
    return 42;
  }).Then([](int y) {
    if (random() % 2) {
      return std::make_error_code(2);
    }
    return y + 15;
  }).Then([](int z) {  // Will not run if we have any error
    return z * 2;
  }).Then([](std::error_code ec) {  // Recover from error codes
    std::cout << ec.value() << std::endl;
    return 10; // some default value
  });
int x = std::move(f).Get().Value();
```
</p></details>

<details><summary>Error recovering from callbacks with Result </summary><p>

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
auto f = yaclib::Run(tp, [] { 
    return 1; 
  }).Then([](int y) {
    if (random() % 2) {
      return std::make_error_code(1);
    }
    return 10;
  }).Then([](int z) {
    if (random() % 2) {
      throw std::runtime_error{""};
    }
    return z * 2;
  }).Then([](yaclib::util::Result<int> res) {
    return 15; 
  }); //  Recover from exception
int x = std::move(f).Get().Value(); // 15
```
</p></details>

<details><summary>Timed wait</summary><p>

```C++
auto tp = yaclib:MakeThreadPool(/*threads=*/4);

yaclib::Future<int> f1 = yaclib::Run(tp, [] { return 42; });
yaclib::Future<double> f2 = yaclib::Run(tp, [] { return 15.0; });

yaclib::WaitFor(10ms, f1, f2);

if (f1.Ready()) {
  Process(std::as_const(f1).Get());
  yaclib::util::Result<int> res1 = std::as_const(f1).Get();
  assert(f1.Valid());  // f1 valid here
}

if (f2.Ready()) {
  Process(std::move(f2).Get());
  assert(!f2.Valid());  // f2 invalid here
}
```
</p></details>

<details><summary>Future unwraping</summary><p>

Sometimes it is necessary to return from one async function the result of the other. It would be possible with the wait on this result. But this would cause to block thread while waiting for the task to complete.

This problem can be solved using future unwrapping: when an async function returns a Future object, instead of setting its result to the Future object, the inner Future will "replace" the outer Future. This means that the outer Future will complete when the inner Future finishes and will acquire the result of the inner Future.

```C++
auto tp_output = yaclib::MakeThreadPool(/*threads=*/1);
auto tp_compute = yaclib::MakeThreadPool(/*threads=CPU cores*/);

auto future = yaclib::Run(tp_output, [] {
  std::cout << "Outer task" <<   std::endl;
  return yaclib::Run(tp_compute, [] { return 42; });
}).Then(/*tp_compute*/ [](int result) {
  result *= 13;
  return yaclib::Run(tp_output, [result] { 
    return std::cout << "Result = " << result << std::endl; 
  });
});
```
</p></details>

<details><summary>Serial Executor, Strand, Async Mutex</summary><p>

```C++
auto tp = MakeThreadPool(4);
// decorated thread pool by serializing tasks:
auto strand = MakeSerial(tp);

size_t counter = 0;

std::vector<std::thread> threads;

for (size_t i = 0; i < 5; ++i) {
  threads.emplace_back([&] {
  for (size_t j = 0; j < 1000; ++j) {
    strand->Execute([&] {
      ++counter; // no data race!
    });
  }
  });
}
```
</p></details>

<details><summary>WhenAll</summary><p>

```C++
auto tp = yaclib::MakeThreadPool(4);
std::vector<yaclib::Future<int>> futs;

// Run sync computations in parallel
for (size_t i = 0; i < 5; ++i) {
  futs.push_back(yaclib::Run(tp, [i]() -> int {
    return random() * i;
  }));
}

// Will be ready when all futures are ready
yaclib::Future<std::vector<int>> all = yaclib::WhenAll(futs.begin(), futs.size());
std::vector<int> unique_ints = std::move(all).Then([](std::vector<int> ints) {
  ints.erase(std::unique(ints.begin(), ints.end()), ints.end());
  return ints;
}).Get().Ok();
```
</p></details>

<a name="req"></a>

## Requirements
### Operating systems

* Linux
* macOS
* Windows
* Android
* iOS (theoretical)

### Compilers
A recent C++ compilers that support C++17
* GCC-9 and later
* Clang-11 and later
* Apple Clang
* MSVC

### Build systems
* CMake

<a name="contrib"></a>

## Contributing
We are always open for issues and pull requests. For more details you can check following links:
* [Specification](https://yaclib.github.io/YACLib)
* [Targets description](doc/target.md)
* [Developer dependencies](doc/dependency.md)
* [PR guide](doc/pr_guide.md)
* [Style guide](doc/style_guide.md)

## Releases
YACLib follows the
[Abseil Live at Head philosophy](https://abseil.io/about/philosophy#upgrade-support).
We recommend using the latest commit in the `main` branch in your projects.

## Contacts
You can contact us by our emails:
* valery.mironow@gmail.com
* kononov.nikolay.nk1@gmail.com
* ionin.code@gmail.com
* zakhar.zakharov.zz16@gmail.com
* myannyax@gmail.com

## License
YACLib is made available under MIT License.
See [LICENSE](LICENSE) file for details

[![FOSSA Status](
https://app.fossa.com/api/projects/git%2Bgithub.com%2FYACLib%2FYACLib.svg?type=large)](
https://app.fossa.com/projects/git%2Bgithub.com%2FYACLib%2FYACLib?ref=badge_large)
