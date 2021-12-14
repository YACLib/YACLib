# Design

## Rationale

**YACLib** is a lightweight C++ library for concurrent and parallel task execution, that is striving to satisfy the
following properties:

* Zero cost abstractions
* Easy to use
* Easy to build
* Good test coverage

### Zero cost abstraction

The abstractions that YACLib provides should make the code optimally written for the particular case, easier and keep it
just as fast.

For example, the creation and execution of a pipeline from a `yaclib::Future` makes at most one allocation for each step
of the pipeline.

### Easy to use

Programming concurrent and parallel programs is difficult, therefore one of the most important goals is to make
**YACLib** easy to use correctly.

Example:

`Future` in our interface has two `Get` overloads:

* ` ReturnValue Get() const & `
* ` ReturnValue Get() && `

and other overloads have been removed.

As a consequence, most of the incorrect or suboptimal use of `Get` is discovered at compile time.

### Easy to build

Building the YACLib should be simple, as otherwise it is difficult to add the library to cross-platform projects.
However, the build should not significantly slow down the build of the target project. Therefore, we build the entire
project using CMake as a static library, trying to use a minimum of public boilerplate code.

### Good test coverage

The most important thing is the absence of bugs. Because it is very difficult to find a multi-threaded bug, and if it is
in the library, it is still more difficult. That's why we:

* 100% line coverage and strive to 100% branch coverage
* Testing the code on multiple platforms with different build flags
* We use static analyzers for C++, such as clang-tidy, cppcheck
* We use dynamic analyzers for C++, such as Google Sanitizers, Valgrind, etc

## Why should I use YACLib instead of ... ?

### std

`future`, `promise` and `packaged_task` are not zero-cost abstractions. For example, to protect result from data
race, `future` uses a mutex instead of a wait-free state machine. Such an overhead is due to the specific API
requirements in the C++ standard. Also std does not have callbacks for `future`, therefore there is no possibility of
their combination.

### OpenMP

Good library for data-based parallelism. Allows you to quickly parallelize computational tasks, but OpenMP is not
suitable for concurrent task execution, which is more common in production.

### oneTBB

This library is a newer and well-written alternative to OpenMP, but it is very big and has a difficult API for
concurrent task execution. Also,
some [code](https://github.com/oneapi-src/oneTBB/blob/master/include/oneapi/tbb/spin_mutex.h#L71) of this library does
not optimally use the C++ memory model and cache coherence protocol.

### Boost.Asio

Good library for network platform-specific API. The main drawback is that CPU-bound thread-pool and other task-based
parallelism primitives (`strand`) are implemented like stubs.

### Folly

This library has a good user-friendly API, but it is large and very difficuilt to build. Also our library
is [faster than](https://github.com/YACLib/Bench) Folly.

### HPX

This library is as big as Folly, but unlike it, Futures are implemented inefficiently, since HPX tries to satisfy the
requirements of the C++ standard.

### Boost.Fiber and Marl

Both libraries provide only user-level threads (fibers), but this is not the only thing that is needed in the library
for concurrent and parallel task execution. In particular, fibers are not always suitable for CPU-bound tasks and for
handle asynchronous callbacks from external libraries.

### Taskflow

It seems that the priority of the library is data-based parallelism by CUDA and OpenCL, but not concurrent task
execution. For example, this library uses `std::future` in CPU-bound thread-pool, and also there is no possibility in
this library to use custom abstractions for a thread.

### libunifex and cppcoro

Both libraries are quite interesting, but experimental. They require C++20 coroutine support. At the moment, libunifex
looks like a proposal, but not as a production ready library. Perhaps it will be ready in C++26, but our library even
supports C++17.

_If you have any thoughts on improving YACLib, feel free to create a Github issue_
