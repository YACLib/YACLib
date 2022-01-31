# YACLib

_Yet Another Concurrency Library_

[![GitHub license](
https://img.shields.io/badge/license-MIT-blue.svg)](
https://raw.githubusercontent.com/YACLib/YACLib/main/LICENSE)
[![FOSSA status](
https://app.fossa.com/api/projects/git%2Bgithub.com%2FYACLib%2FYACLib.svg?type=shield)](
https://app.fossa.com/projects/git%2Bgithub.com%2FYACLib%2FYACLib)

[![Linux](
https://github.com/YACLib/YACLib/actions/workflows/linux.yml/badge.svg?branch=main)](
https://github.com/YACLib/YACLib/actions/workflows/linux.yml)
[![macOS](
https://github.com/YACLib/YACLib/actions/workflows/macos.yml/badge.svg?branch=main)](
https://github.com/YACLib/YACLib/actions/workflows/macos.yml)
[![Windows](
https://github.com/YACLib/YACLib/actions/workflows/windows.yml/badge.svg?branch=main)](
https://github.com/YACLib/YACLib/actions/workflows/windows.yml)
[![Sanitizers](
https://github.com/YACLib/YACLib/actions/workflows/sanitizer.yml/badge.svg?branch=main)](
https://github.com/YACLib/YACLib/actions/workflows/sanitizer.yml)

[![Test coverage](
https://codecov.io/gh/YACLib/YACLib/branch/main/graph/badge.svg)](
https://codecov.io/gh/YACLib/YACLib)
[![Codacy Badge](
https://app.codacy.com/project/badge/Grade/4113686840a645a8950abdf1197611bd)](
https://www.codacy.com/gh/YACLib/YACLib/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=YACLib/YACLib&amp;utm_campaign=Badge_Grade)

[![Discord](
https://discordapp.com/api/guilds/898966884471423026/widget.png)](
https://discord.gg/xy2fDKj8VZ)

## Table of Contents

* [About YACLib](#about-yaclib)
* [Getting started](#getting-started)
* [Examples](#examples)
    * [Asynchronous pipeline](#asynchronous-pipeline)
    * [Thread Pool](#thread-pool)
    * [Strand, Serial Executor, AsyncMutex](#strand-serial-executor-async-mutex)
    * [WhenAll](#whenall)
    * [WhenAny](#whenany)
    * [Future Unwrapping](#future-unwrapping)
    * [Timed wait](#timed-wait)
    * [Exception recovering](#exception-recovering)
    * [Error recovering](#error-recovering)
    * [Using Result for smart recovering](#use-result-for-smart-recovering)
* [Requirements](#requirements)
* [Releases](#releases)
* [Contributing](#contributing)
* [Contacts](#contacts)
* [License](#license)

## About YACLib

**YACLib** is a lightweight C++ library for concurrent and parallel task execution, that is striving to satisfy the
following properties:

* Zero cost abstractions
* Easy to use
* Easy to build
* Good test coverage

For more details check our [design document](doc/design.md) and [documentation](https://yaclib.github.io/YACLib).

## Getting started

For quick start just paste this code in your `CMakeLists.txt` file.

```CMake
include(FetchContent)
FetchContent_Declare(yaclib
  GIT_REPOSITORY https://github.com/YACLib/YACLib.git
  GIT_TAG main
  )
FetchContent_MakeAvailable(yaclib)
link_libraries(yaclib)
```

For more details check [install guide](doc/install.md).

## Examples

<details open><summary> 
Here are short examples of using some features from YACLib, for details
check <a href="https://yaclib.github.io/YACLib/examples.html">documentation</a>.
</summary>

#### Asynchronous pipeline

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
yaclib::Run(tp, [] {
    return 42;
  }).Then([](int r) {
    return r * 2;
  }).Then([](int r) {
    return r + 1; 
  }).Then([](int r) {
    return std::to_string(r);
  }).Subscribe([](std::string r) {
    std::cout << "Pipeline result: <"  << r << ">" << std::endl;
  });
};
```

#### Thread Pool

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
tp->Execute([] {
  // some computations...
});
tp->Execute([] {
  // some computations...
});

tp->Stop();
tp->Wait();
```

#### Strand, Serial Executor, Async Mutex

```C++
auto tp = yaclib::MakeThreadPool(4);
// decorated thread pool by serializing tasks:
auto strand = yaclib::MakeStrand(tp);

size_t counter = 0;
auto tp2 = yaclib::MakeThreadPool(4);

for (size_t i = 0; i < 100; ++i) {
  tp2->Execute([&] {
    strand->Execute([&] {
      ++counter; // no data race!
    });
  });
}
```

#### WhenAll

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
std::vector<yaclib::Future<int>> fs;

// Run parallel computations
for (size_t i = 0; i < 5; ++i) {
  fs.push_back(yaclib::Run(tp, [i]() -> int {
    return random() * i;
  }));
}

// Will be ready when all futures are ready
yaclib::Future<std::vector<int>> all = yaclib::WhenAll(fs.begin(), fs.size());
std::vector<int> unique_ints = std::move(all).Then([](std::vector<int> ints) {
  ints.erase(std::unique(ints.begin(), ints.end()), ints.end());
  return ints;
}).Get().Ok();
```

#### WhenAny

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
std::vector<yaclib::Future<int>> fs;

// Run parallel computations
for (size_t i = 0; i < 5; ++i) {
  fs.push_back(yaclib::Run(tp, [i] {
    // connect with one of the database shard
    return i;
  }));
}

// Will be ready when any future is ready
yaclib::WhenAny(fs.begin(), fs.size()).Subscribe([](int i) {
  // some work with database
});
```

#### Future unwrapping

Sometimes it is necessary to return from one async function the result of the other. It would be possible with the wait
on this result. But this would cause blocking thread while waiting for the task to complete.

This problem can be solved using future unwrapping: when an async function returns a Future object, instead of setting
its result to the Future object, the inner Future will "replace" the outer Future. This means that the outer Future will
complete when the inner Future finishes and will acquire the result of the inner Future.

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

#### Timed wait

```C++
auto tp = yaclib:MakeThreadPool(/*threads=*/4);

yaclib::Future<int> f1 = yaclib::Run(tp, [] { return 42; });
yaclib::Future<double> f2 = yaclib::Run(tp, [] { return 15.0; });

yaclib::WaitFor(10ms, f1, f2);  // or yaclib::Wait / yaclib::WaitUntil

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

#### Exception recovering

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
auto f = yaclib::Run(tp, [] {
    if (random() % 2) {
      throw std::runtime_error{"1"};
    }
    return 42;
  }).Then([](int y) {
    if (random() % 2) {
      throw std::runtime_error{"2"};
    }
    return y + 15;
  }).Then([](int z) {  // Will not run if we have any error
    return z * 2;
  }).Then([](std::exception_ptr e) {  // Recover from error codes
    try {
      std::rethrow_exception(e);
    } catch (const std::runtime_error& e) {
      std::cout << e.what() << std::endl;
    }
    return 10;  // Some default value
  });
int x = std::move(f).Get().Value();
```

#### Error recovering

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
    return 10;  // Some default value
  });
int x = std::move(f).Get().Value();
```

#### Use Result for smart recovering

```C++
auto tp = yaclib::MakeThreadPool(/*threads=*/4);
auto f = yaclib::Run(tp, [] {
    if (random() % 2) {
      return std::make_error_code(1);
    }
    return 42;
  }).Then([](int y) {
    if (random() % 2) {
      throw std::runtime_error{"2"};
    }
    return y + 15;
  }).Then([](yaclib::util::Result<int> z) {
    if (!z) {
      return 10; // Some default value
    }
    return z.Value(); 
  });
int x = std::move(f).Get().Value();
```

</details>

## Requirements

YACLib is a static library, that uses _CMake_ as a build system and requires a compiler with C++17 or newer.

If the library doesn't compile on some compiler satisfying this condition, please create an issue. Pull requests with
fixes are welcome!

We can also try to support older standards. If you are interested in it, check
this [discussion](https://github.com/YACLib/YACLib/discussions/102).

We test following configurations:

✅ - CI tested

👌 - manually tested

| OS\Compiler | Linux | Windows   | macOS | Android |
|-------------|-------|-----------|-------|---------|
| GCC         | ✅ 7+  | ✅ MinGW   | ✅ 9+  | 👌      |
| Clang       | ✅ 8+  | ✅ ClangCL |       | 👌      |
| AppleClang  | —     | —         | ✅ 12+ | —       |
| MSVC        | —     | ✅ 14.20+  | —     | —       |

## Releases

YACLib follows the [Abseil Live at Head philosophy](https://abseil.io/about/philosophy#upgrade-support)
(update to the latest commit from the main branch as often as possible).

So we recommend using the latest commit in the main branch in your projects.

However, we realize this philosophy doesn't work for every project, so we also provide Releases.

## Contributing

We are always open for issues and pull requests. For more details you can check following links:

* [Specification](https://yaclib.github.io/YACLib)
* [Targets description](doc/target.md)
* [Dev dependencies](doc/dependency.md)
* [PR guide](doc/pr_guide.md)
* [Style guide](doc/style_guide.md)
* [How to use sanitizers](doc/sanitizer.md)

## Contacts

You can contact us by our emails:

* valery.mironow@gmail.com
* kononov.nikolay.nk1@gmail.com
* ionin.code@gmail.com
* zakhar.zakharov.zz16@gmail.com
* myannyax@gmail.com

Or join our [Discord Server](https://discord.gg/xy2fDKj8VZ)

## License

YACLib is made available under MIT License. See [LICENSE](LICENSE) file for details.

We would be glad if you let us know that you're using our library.

[![FOSSA Status](
https://app.fossa.com/api/projects/git%2Bgithub.com%2FYACLib%2FYACLib.svg?type=large)](
https://app.fossa.com/projects/git%2Bgithub.com%2FYACLib%2FYACLib?ref=badge_large)
