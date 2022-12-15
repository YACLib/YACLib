#pragma once

#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
#include <yaclib/lazy/schedule.hpp>

#if YACLIB_CORO != 0
#  include <yaclib/coro/future.hpp>
#  include <yaclib/coro/task.hpp>
#endif

#include <string>

#include <gtest/gtest.h>

namespace test {

template <typename T>
struct AsyncSuite : testing::Test {
  static constexpr bool kIsFuture = yaclib::is_future_base_v<T>;
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
      default:
        return "unknown";
    }
  }
};

using Types = testing::Types<yaclib::Future<>, yaclib::Task<>>;

TYPED_TEST_SUITE(AsyncSuite, Types, TypeNames);

#define INVOKE(executor, func)                                                                                         \
  [&] {                                                                                                                \
    if constexpr (TestFixture::kIsFuture) {                                                                            \
      return yaclib::Run(executor, func);                                                                              \
    } else {                                                                                                           \
      return yaclib::Schedule(executor, func);                                                                         \
    }                                                                                                                  \
  }()

#define INVOKE_V(V, executor, func)                                                                                    \
  [&] {                                                                                                                \
    if constexpr (TestFixture::kIsFuture) {                                                                            \
      return yaclib::AsyncContract<V>(executor, func);                                                                 \
    } else {                                                                                                           \
      return yaclib::LazyContract<V>(executor, func);                                                                  \
    }                                                                                                                  \
  }()

#define DETACH(future, func)                                                                                           \
  do {                                                                                                                 \
    if constexpr (TestFixture::kIsFuture) {                                                                            \
      std::move(future).Detach(func);                                                                                  \
    } else {                                                                                                           \
      std::move(future).Then(func).Detach();                                                                           \
    }                                                                                                                  \
  } while (false)

}  // namespace test
