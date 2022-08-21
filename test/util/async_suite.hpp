#include <yaclib/async/future.hpp>
#include <yaclib/async/task.hpp>

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

#define DETACH(future, func)                                                                                           \
  do {                                                                                                                 \
    if constexpr (TestFixture::kIsFuture) {                                                                            \
      std::move(future).Detach(func);                                                                                  \
    } else {                                                                                                           \
      std::move(future).Then(func).Detach();                                                                           \
    }                                                                                                                  \
  } while (false)

#define GET_READY(type, value)                                                                                         \
  [&] {                                                                                                                \
    if constexpr (TestFixture::kIsFuture) {                                                                            \
      return yaclib::MakeFuture<type>(value);                                                                          \
    } else {                                                                                                           \
      return yaclib::Schedule([v = std::move(value)] {                                                                 \
        return std::move(v);                                                                                           \
      });                                                                                                              \
    }                                                                                                                  \
  }()

}  // namespace test
