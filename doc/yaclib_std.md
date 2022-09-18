## Fault injection

Inspired mainly by https://gitlab.com/Lipovsky/twist (mostly we rewrote it to achieve our goals), and also: 
* https://github.com/ClickHouse/ClickHouse/blob/master/src/Common/ThreadFuzzer.h
* https://github.com/dvyukov/relacy
* https://github.com/apple/foundationdb/tree/main/flow
* papers about fault injection and model checking

#### Options

* `YACLIB_FAULT=OFF`: 
  
  Just using for `::std` primitives

* `YACLIB_FAULT=THREAD`: 

  Thin wrappers for `::std` primitives with fault injection calls (sleep_for, yield etc.) in some of the methods

  So it should be able to work in any environment, for example you cannot replace all primitives for `yaclib_std::`

* `YACLIB_FAULT=FIBER`:

  Single thread cooperative, deterministic execution, trying to be `::std` compatible.

  Using fiber instead of `std::thread` and cooperative primitives for it with our own fiber scheduler.

* TODO(MBkkt) Write about runtime configuration options


#### Advantages

* You can just grep for header names `<thread>`, `<atomic>` etc. and
  replace them with`<yaclib_std/thread>`, `<yaclib_std/atomic>`.
  Similarly you can replace names from the standard library, e.g.
  `std::thread`, `std::atomic` etc. and replace them with YACLib equivalents
  `yaclib_std::thread`, `yaclib_std::atomic`, ....

  And everything will just work.

* `FIBER` backend adopted to easily run with any test framework, check our [test main](https://github.com/YACLib/YACLib/blob/main/test/test.cpp)

* Trying to be fully C++17 `::std` compatible, except `std::future`/etc, because we suggest YACLib as drop-in alternative

