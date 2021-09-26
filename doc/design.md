# Design

## Rationale:

YACLib - C++ library for concurrent and parallel task execution, which is an alternative to existing solutions, striving
to satisfy the following properties:

* Easy to use
* Zero cost abstraction
* Easy to build
* Good test coverage

### Easy to use:

Programming concurrent/parallel programs is difficult, therefore one of the most important goals, to make YACLib easy to
use correctly.

Example:

`Future` in our interface has two `Get` overloads:` ReturnValue Get() const & `and` ReturnValue Get() && `, and the
overloads `ReturnValue Get() &` and `ReturnValue Get() const &&` have been removed.

As a consequence, most of the incorrect or suboptimal use of `Get` is discovered at compile time.

### Zero cost abstraction:

The abstractions that YACLib provides should make the code optimally written for the particular case, easier and keep it
just as fast.

For example, therefore, `Serial::Execute(...some task...)` is lock-free, and the creation and execution of a pipeline
from a Future makes exactly one allocation for each step of the pipeline.

### Easy to build:

Building the YACLib should be simple, as otherwise it is difficult to add the library to cross-platform projects.
However, the build should not significantly slow down the build of the target project. Therefore, we build the entire
project using CMake as a static library, trying to use a minimum of public boilerplate code.

### Good test coverage:

The most important thing is the absence of bugs. Because it is very difficult to find a multi-threaded bug, and if it is
in the library, it is still more difficult. That's why we:

* Strive for 100% test coverage
* Testing the code on multiple platforms with different build flags
* We use static analyzers for C++, such as clang-tidy, cppcheck
* We use dynamic analyzers for C++, such as Google Sanitizers, Valgrind, etc
