#pragma once

#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
#include <yaclib/lazy/schedule.hpp>

#if YACLIB_CORO != 0
#  include <yaclib/coro/future.hpp>
#  include <yaclib/coro/shared_future.hpp>
#  include <yaclib/coro/task.hpp>
#endif

#include <string>

#include <gtest/gtest.h>

namespace test {

template <typename T>
struct AsyncSuite : testing::Test {
  static constexpr bool kIsFuture = yaclib::is_future_base_v<T>;
  static constexpr bool kIsTask = yaclib::is_task_v<T>;
  static constexpr bool kIsSharedFuture = yaclib::is_shared_future_base_v<T>;

  template <typename V = void, typename T2 = yaclib::DefaultTrait>
  using FutureT = std::conditional_t<kIsSharedFuture, yaclib::SharedFuture<V, T2>, yaclib::Future<V, T2>>;

  template <typename V = void, typename T2 = yaclib::DefaultTrait>
  using FutureOnT = std::conditional_t<kIsSharedFuture, yaclib::SharedFutureOn<V, T2>, yaclib::FutureOn<V, T2>>;

  template <typename V = void, typename T2 = yaclib::DefaultTrait>
  using AsyncT = std::conditional_t<kIsTask, yaclib::Task<V, T2>, FutureT<V, T2>>;

  using Type = T;
};

struct TypeNames {
  template <typename T>
  static std::string GetName(int i) {
    switch (i) {
      case 0:
        return "Future";
      case 1:
        return "Task";
      case 2:
        return "SharedFuture";
      default:
        return "unknown";
    }
  }
};

using Types = testing::Types<yaclib::Future<>, yaclib::Task<>, yaclib::SharedFuture<>>;

TYPED_TEST_SUITE(AsyncSuite, Types, TypeNames);

template <typename T>
struct FutureSuite : testing::Test {
  static constexpr bool kIsFuture = yaclib::is_future_base_v<T>;
  static constexpr bool kIsSharedFuture = yaclib::is_shared_future_base_v<T>;

  template <typename V = void, typename T2 = yaclib::DefaultTrait>
  using FutureT = std::conditional_t<kIsFuture, yaclib::Future<V, T2>, yaclib::SharedFuture<V, T2>>;

  template <typename V = void, typename T2 = yaclib::DefaultTrait>
  using FutureOnT = std::conditional_t<kIsFuture, yaclib::FutureOn<V, T2>, yaclib::SharedFutureOn<V, T2>>;
};

struct FutureTypeNames {
  template <typename T>
  static std::string GetName(int i) {
    switch (i) {
      case 0:
        return "Future";
      case 1:
        return "SharedFuture";
      default:
        return "unknown";
    }
  }
};

using FutureTypes = testing::Types<yaclib::Future<>, yaclib::SharedFuture<>>;

TYPED_TEST_SUITE(FutureSuite, FutureTypes, FutureTypeNames);

#define INVOKE(executor, func)                                                                                         \
  [&] {                                                                                                                \
    if constexpr (TestFixture::kIsFuture) {                                                                            \
      return yaclib::Run(executor, func);                                                                              \
    } else if constexpr (TestFixture::kIsTask) {                                                                       \
      return yaclib::Schedule(executor, func);                                                                         \
    } else {                                                                                                           \
      return yaclib::RunShared(executor, func);                                                                        \
    }                                                                                                                  \
  }()

#define RUN(executor, func)                                                                                            \
  [&] {                                                                                                                \
    if constexpr (TestFixture::kIsSharedFuture) {                                                                      \
      return yaclib::RunShared(executor, func);                                                                        \
    } else {                                                                                                           \
      return yaclib::Run(executor, func);                                                                              \
    }                                                                                                                  \
  }()

#define INVOKE_V(V, executor, func)                                                                                    \
  [&] {                                                                                                                \
    if constexpr (TestFixture::kIsFuture) {                                                                            \
      return yaclib::AsyncContract<V>(executor, func);                                                                 \
    } else if constexpr (TestFixture::kIsTask) {                                                                       \
      return yaclib::LazyContract<V>(executor, func);                                                                  \
    } else {                                                                                                           \
      return yaclib::AsyncSharedContract<V>(executor, func);                                                           \
    }                                                                                                                  \
  }()

#define DETACH(future, func)                                                                                           \
  do {                                                                                                                 \
    if constexpr (TestFixture::kIsFuture) {                                                                            \
      std::move(future).Detach(func);                                                                                  \
    } else if constexpr (TestFixture::kIsSharedFuture) {                                                               \
      future.Subscribe(func);                                                                                          \
      std::move(future).Detach();                                                                                      \
    } else {                                                                                                           \
      std::move(future).Then(func).Detach();                                                                           \
    }                                                                                                                  \
  } while (false)

#define PROMISE_T(type)                                                                                                \
  std::conditional_t<TestFixture::kIsSharedFuture, yaclib::SharedPromise<type>, yaclib::Promise<type>>

}  // namespace test
