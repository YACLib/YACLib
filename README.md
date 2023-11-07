# YACLib

[Yet Another Concurrency Library](https://github.com/YACLib/YACLib)

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

[![Test coverage: coveralls](
https://coveralls.io/repos/github/YACLib/YACLib/badge.svg?branch=main)](
https://coveralls.io/github/YACLib/YACLib?branch=main)
[![Test coverage: codecov](
https://codecov.io/gh/YACLib/YACLib/branch/main/graph/badge.svg)](
https://codecov.io/gh/YACLib/YACLib)

<!-- 
Codacy doesn't work for some reason
[![Codacy Badge](
https://app.codacy.com/project/badge/Grade/4113686840a645a8950abdf1197611bd)](
https://www.codacy.com/gh/YACLib/YACLib/dashboard?utm_source=github.com&amp;utm_medium=referral&amp;utm_content=YACLib/YACLib&amp;utm_campaign=Badge_Grade)
-->

[![Discord](
https://discordapp.com/api/guilds/898966884471423026/widget.png)](
https://discord.gg/xy2fDKj8VZ)

## Table of Contents

* [About YACLib](#about-yaclib)
* [Getting started](#getting-started)
* [Examples](#examples)
    * [Asynchronous pipeline](#asynchronous-pipeline)
    * [C++20 coroutine](#c20-coroutine)
    * [Lazy pipeline](#lazy-pipeline)
    * [Thread pool](#thread-pool)
    * [Strand, Serial executor](#strand-serial-executor)
    * [Mutex](#mutex)
    * [Rescheduling](#rescheduling)
    * [WhenAll](#whenall)
    * [WhenAny](#whenany)
    * [Future unwrapping](#future-unwrapping)
    * [Timed wait](#timed-wait)
    * [WaitGroup](#waitgroup)
    * [Exception recovering](#exception-recovering)
    * [Error recovering](#error-recovering)
    * [Using Result for smart recovering](#use-result-for-smart-recovering)
* [Requirements](#requirements)
* [Releases](#releases)
* [Contributing](#contributing)
* [Thanks](#thanks)
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

For more details about 'yaclib_std' or fault injection, check [doc](doc/yaclib_std.md).

## Examples

Here are short examples of using some features from YACLib, for details
check [documentation](https://yaclib.github.io/YACLib/examples.html).

#### Asynchronous pipeline

```cpp
yaclib::FairThreadPool cpu_tp{/*threads=*/4};
yaclib::FairThreadPool io_tp{/*threads=*/1};

yaclib::Run(cpu_tp, [] {  // on cpu_tp
  return 42;
}).ThenInline([](int r) {  // called directly after 'return 42', without Submit to cpu_tp
  return r + 1;
}).Then([](int r) {  // on cpu_tp
  return std::to_string(r);
}).Detach(io_tp, [](std::string&& r) {  // on io_tp
  std::cout << "Pipeline result: <"  << r << ">" << std::endl; // 43
});
```

We guarantee that no more than one allocation will be made for each step of the pipeline.

We have `Then/Detach` x `IExecutor/previous step IExecutor/Inline`.

Also Future/Promise don't contain shared atomic counters!

#### C++20 coroutine

```cpp
yaclib::Future<int> task42() {
  co_return 42;
}

yaclib::Future<int> task43() {
  auto value = co_await task42();
  co_return value + 1;
}
```

You can zero cost-combine Future coroutine code with Future callbacks code.
That allows using YAClib for a smooth transfer from C++17 to C++20 with coroutines.

Also Future with coroutine doesn't make additional allocation for Future,
only coroutine frame allocation that is caused by compiler,
and [can be optimized](https://www.open-std.org/jtc1/sc22/wg21/docs/papers/2018/p0981r0.html).

And finally `co_await` doesn't require allocation,
so you can combine some async operation without allocation.

#### Lazy pipeline

```cpp
auto task = yaclib::Schedule(tp1, [] {
  return 1; 
}).Then([] (int x) {
  return x * 2;
});

task.Run(); // Run task on tp1
```

Same as asynchronous pipeline, but starting only after Run/ToFuture/Get.
Task can be used as coroutine return type too.

Also running a Task that returns a Future doesn't make allocation.
And it doesn't need synchronization, so it is even faster than asynchronous pipeline.

#### Thread pool

```cpp
yaclib::FairThreadPool tp{/*threads=*/4};
Submit(tp, [] {
  // some computations...
});
Submit(tp, [] {
  // some computations...
});

tp.Stop();
tp.Wait();
```

#### Strand, Serial executor

```cpp
yaclib::FairThreadPool cpu_tp{4};  // thread pool for cpu tasks
yaclib::FairThreadPool io_tp{1};   // thread pool for io tasks
auto strand = yaclib::MakeStrand(&tp);

for (std::size_t i = 0; i < 100; ++i) {
  yaclib::Run(cpu_tp, [] {
    // ... parallel computations ...
  }).Then(strand, [](auto result) {
    // ... critical section ...
  }).Then(io_tp, [] {
    // ... io tasks ...
  }).Detach();
}
```

This is much more efficient than a mutex because 
1. don't block the threadpool thread.
1. we execute critical sections in batches (the idea is known as flat-combining).

And also the implementation of strand is lock-free and efficient, without additional allocations.

#### Mutex

```cpp
yaclib::FairThreadPool cpu_tp{4};  // thread pool for cpu tasks
yaclib::FairThreadPool io_tp{1};   // thread pool for io tasks
yaclib::Mutex<> m;

auto compute = [&] () -> yaclib::Future<> {
  co_await On(tp);
  // ... parallel computations ...
  auto guard = co_await m.Lock();
  // ... critical section ...
  co_await guard.UnlockOn(io_tp);
  // ... io tasks ...
};

for (std::size_t i = 0; i < 100; ++i) {
  compute().Detach();
}
```

First, this is the only correct mutex implementation for C++20 coroutines
as far as I know (cppcoro, libunifex, folly::coro implement Unlock incorrectly, it serializes the code after Unlock)

Second, `Mutex` inherits all the `Strand` benefits.

#### Rescheduling

```cpp
yaclib::Future<> bar(yaclib::IExecutor& cpu, yaclib::IExecutor& io) {
  co_await On(cpu);
  // ... some heavy computation ...
  co_await On(io);
  // ... some io computation ...
}
```

This is really zero-cost, just suspend the coroutine and submit its resume to another executor,
without synchronization inside the coroutine and allocations anywhere.

#### WhenAll

```cpp
yaclib::FairThreadPool tp{/*threads=*/4};
std::vector<yaclib::Future<int>> fs;

// Run parallel computations
for (std::size_t i = 0; i < 5; ++i) {
  fs.push_back(yaclib::Run(tp, [i]() -> int {
    return random() * i;
  }));
}

// Will be ready when all futures are ready
yaclib::Future<std::vector<int>> all = WhenAll(fs.begin(), fs.size());
std::vector<int> unique_ints = std::move(all).Then([](std::vector<int> ints) {
  ints.erase(std::unique(ints.begin(), ints.end()), ints.end());
  return ints;
}).Get().Ok();
```

Doesn't make more than 3 allocations regardless of input size.

#### WhenAny

```cpp
yaclib::FairThreadPool tp{/*threads=*/4};
std::vector<yaclib::Future<int>> fs;

// Run parallel computations
for (std::size_t i = 0; i < 5; ++i) {
  fs.push_back(yaclib::Run(tp, [i] {
    // connect with one of the database shards
    return i;
  }));
}

// Will be ready when any future is ready
WhenAny(fs.begin(), fs.size()).Detach([](int i) {
  // some work with database
});
```

Doesn't make more than 2 allocations regardless of input size.

#### Future unwrapping

```cpp
yaclib::FairThreadPool tp_output{/*threads=*/1};
yaclib::FairThreadPool tp_compute{/*threads=CPU cores*/};

auto future = yaclib::Run(tp_output, [] {
  std::cout << "Outer task" << std::endl;
  return yaclib::Run(tp_compute, [] { return 42; });
}).Then(/*tp_compute*/ [](int result) {
  result *= 13;
  return yaclib::Run(tp_output, [result] { 
    std::cout << "Result = " << result << std::endl; 
  });
});
```

Sometimes it's necessary to return from one async function the result of the other. It would be possible with the wait
on this result. But this would cause blocking of the thread while waiting for the task to complete.

This problem can be solved using future unwrapping: when an async function returns a Future object, instead of setting
its result to the Future object, the inner Future will "replace" the outer Future. This means that the outer Future will
complete when the inner Future finishes and will acquire the result of the inner Future.

It also doesn't require additional allocations.

#### Timed wait

```cpp
yaclib::FairThreadPool tp{/*threads=*/4};

yaclib::Future<int> f1 = yaclib::Run(tp, [] { return 42; });
yaclib::Future<double> f2 = yaclib::Run(tp, [] { return 15.0; });

WaitFor(10ms, f1, f2);  // or Wait / WaitUntil

if (f1.Ready()) {
  Process(std::as_const(f1).Get());
  yaclib::Result<int> res1 = std::as_const(f1).Get();
  assert(f1.Valid());  // f1 valid here
}

if (f2.Ready()) {
  Process(std::move(f2).Get());
  assert(!f2.Valid());  // f2 invalid here
}
```

We support `Wait/WaitFor/WaitUntil`.
Also all of them don't make allocation, and we have optimized the path for single `Future` (used in `Future::Get()`).

#### WaitGroup

```cpp
yaclib::WaitGroup wg{1};

yaclib::FairThreadPool tp;

wg.Add(2/*default=1*/);
Submit(tp, [] {
   wg.Done();
});
Submit(tp, [] {
   wg.Done();
});

yaclib::Future<int> f1 = yaclib::Run(tp, [] {...});
wg.Attach(f1);  // auto Done then Future became Ready

yaclib::Future<> f2 = yaclib::Run(tp, [] {...});
wg.Consume(std::move(f2));  // auto Done then Future became Ready

auto coro = [&] () -> yaclib::Future<> {
  co_await On(tp);
  co_await wg; // alias for co_await wg.Await(CurrentThreadPool());
  std::cout << f1.Touch().Ok(); // Valid access to Result of Ready Future
};

auto coro_f = coro();

wg.Done(/*default=1*/);
wg.Wait();
```

Effective like simple atomic counter in intrusive pointer, also doesn't require any allocation.

#### Exception recovering

```cpp
yaclib::FairThreadPool tp{/*threads=*/4};

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

```cpp
yaclib::FairThreadPool tp{/*threads=*/4};

auto f = yaclib::Run<std::error_code>(tp, [] {
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

```cpp
yaclib::FairThreadPool tp{/*threads=*/4};

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
}).Then([](yaclib::Result<int>&& z) {
  if (!z) {
    return 10;  // Some default value
  }
  return std::move(z).Value();
});
int x = std::move(f).Get().Value();
```

## Requirements

YACLib is a static library, that uses _CMake_ as a build system and requires a compiler with C++17 or newer.

If the library doesn't compile on some compiler satisfying this condition, please create an issue. Pull requests with
fixes are welcome!

We can also try to support older standards. If you are interested in it, check
this [discussion](https://github.com/YACLib/YACLib/discussions/102).

We test following configurations:

✅ - CI tested

👌 - manually tested

| Compiler\OS | Linux | Windows   | macOS | Android |
|-------------|-------|-----------|-------|---------|
| GCC         | ✅ 7+  | 👌 MinGW  | ✅ 7+  | 👌      |
| Clang       | ✅ 8+  | ✅ ClangCL | ✅ 8+  | 👌      |
| AppleClang  | —     | —         | ✅ 12+ | —       |
| MSVC        | —     | ✅ 14.20+  | —     | —       |

MinGW works in CI early, check [this](https://github.com/YACLib/YACLib/issues/190).

## Releases

YACLib follows the [Abseil Live at Head philosophy](https://abseil.io/about/philosophy#upgrade-support)
(update to the latest commit from the main branch as often as possible).

So we recommend using the latest commit in the main branch in your projects.

This is safe because we suggest compiling YACLib from source,
and each commit in main goes through dozens of test runs in various configurations.
Our test coverage is 100%, to simplify, we run tests on the cartesian product of possible configurations:

`os x compiler x stdlib x sanitizer x fault injection backend`

However, we realize this philosophy doesn't work for every project,
so we also provide [Releases](https://github.com/YACLib/YACLib/releases).

We don't believe in [SemVer](https://semver.org) (check [this](https://gist.github.com/jashkenas/cbd2b088e20279ae2c8e)),
but we use a `year.month.day[.patch]` versioning approach.
I'll release a new version if you ask, or I'll decide we have important or enough changes.

## Contributing

We are always open for issues and pull requests.
Check our [good first issues](
https://github.com/YACLib/YACLib/issues?q=is%3Aopen+is%3Aissue+label%3A%22good+first+issue%22).

For more details you can check the following links:

* [Specification](https://yaclib.github.io/YACLib)
* [Targets description](doc/target.md)
* [Dev dependencies](doc/dependency.md)
* [Style guide](doc/style_guide.md)
* [How to use sanitizers](doc/sanitizer.md)

## Thanks

* [Roman Lipovsky](https://gitlab.com/Lipovsky) for an incredible
  [course about concurrency](https://gitlab.com/Lipovsky/concurrency-course),
  which gave us a lot of ideas for this library and for showing us how important to test concurrency correctly.

* Paul E. McKenney for an incredible
  [book about parallel programming](https://cdn.kernel.org/pub/linux/kernel/people/paulmck/perfbook/perfbook.html),
  which gave me a lot of insight into memory models and how they relate to what's going on in hardware.

## Contacts

You can contact us by my email: valery.mironow@gmail.com

Or join our [Discord Server](https://discord.gg/xy2fDKj8VZ)

## License

YACLib is made available under MIT License. See [LICENSE](LICENSE) file for details.

We would be glad if you let us know that you're using our library.

[![FOSSA Status](
https://app.fossa.com/api/projects/git%2Bgithub.com%2FYACLib%2FYACLib.svg?type=large)](
https://app.fossa.com/projects/git%2Bgithub.com%2FYACLib%2FYACLib?ref=badge_large)
