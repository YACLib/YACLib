#include <yaclib/async/run.hpp>
#include <yaclib/async/when_any.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <iostream>
#include <string>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;
using namespace std::chrono_literals;

enum class TestSuite { Vector, Array };

template <TestSuite suite, typename T = int>
void JustWorks() {
  constexpr int kSize = 3;
  std::array<async::Promise<T>, kSize> promises;
  std::array<async::Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = async::MakeContract<T>();
    futures[i] = std::move(f);
    promises[i] = std::move(p);
  }

  auto any = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return async::WhenAny(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return async::WhenAny(futures.begin(), futures.end());
    }
  }();

  EXPECT_FALSE(any.Ready());

  if constexpr (std::is_void_v<T>) {
    std::move(promises[1]).Set();
  } else {
    std::move(promises[1]).Set(5);
  }

  EXPECT_TRUE(any.Ready());

  if constexpr (!std::is_void_v<T>) {
    std::move(promises[1]).Set(100);
    EXPECT_EQ(std::move(any).Get().Ok(), 5);
  }
}

TEST(Vector, JustWorks) {
  JustWorks<TestSuite::Vector>();
}

TEST(VoidVector, JustWorks) {
  JustWorks<TestSuite::Vector, void>();
}

TEST(Array, JustWorks) {
  JustWorks<TestSuite::Array>();
}

TEST(VoidArray, JustWorks) {
  JustWorks<TestSuite::Array, void>();
}

template <TestSuite suite, typename T = void>
void AllFails() {
  constexpr int kSize = 3;
  std::array<async::Promise<T>, kSize> promises;
  std::array<async::Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = async::MakeContract<T>();
    futures[i] = std::move(f);
    promises[i] = std::move(p);
  }

  auto any = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return async::WhenAny(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return async::WhenAny(futures.begin(), futures.end());
    }
  }();

  EXPECT_FALSE(any.Ready());

  std::move(promises[1]).Set(std::make_exception_ptr(std::runtime_error{""}));

  EXPECT_TRUE(any.Ready());

  // Second error
  std::move(promises[0]).Set(std::error_code{});

  EXPECT_THROW(std::move(any).Get().Ok(), std::runtime_error);
}

TEST(Vector, AllFails) {
  AllFails<TestSuite::Vector>();
}

TEST(VoidVector, AllFails) {
  AllFails<TestSuite::Vector, void>();
}

TEST(Array, AllFails) {
  AllFails<TestSuite::Array>();
}

TEST(VoidArray, AllFails) {
  AllFails<TestSuite::Array, void>();
}

template <typename T = int>
void EmptyInput() {
  util::Result<T> res{};
  auto empty = std::vector<async::Future<T>>{};
  auto any = async::WhenAny(empty.begin(), empty.end());

  EXPECT_TRUE(any.Ready());
  EXPECT_NO_THROW(std::move(any).Get().Ok());
}

// TEST(Vector, EmptyInput) {
//   EmptyInput();
// }

TEST(VoidVector, EmptyInput) {
  EmptyInput<void>();
}

template <TestSuite suite, typename T = int>
void MultiThreaded() {
  static constexpr bool is_void = std::is_void_v<T>;

  auto tp = executor::MakeThreadPool(4);

  auto async_value = [tp](int value) {
    return async::Run(tp, [value] {
      std::this_thread::sleep_for(10ms);
      if constexpr (!is_void) {
        return value;
      } else {
        (void)value;
      }
    });
  };

  static const int kValues = 6;
  static const std::set<int> kOuts = {0, 1, 2, 3, 4, 5};

  std::array<async::Future<T>, 6> fs;
  for (int i = 0; i < kValues; ++i) {
    fs[i] = async_value(i);
  }

  auto ints =
      [&fs] {
        if constexpr (suite == TestSuite::Vector) {
          return async::WhenAny(fs.begin(), fs.end());
        } else {
          return async::WhenAny(std::move(fs[0]), std::move(fs[1]), std::move(fs[2]), std::move(fs[3]),
                                std::move(fs[4]), std::move(fs[5]));
        }
      }()
          .Get();

  if constexpr (is_void) {
    EXPECT_NO_THROW(std::move(ints).Ok());
  } else {
    auto result = std::move(ints).Ok();
    EXPECT_TRUE(kOuts.find(result) != kOuts.end());
  }

  tp->HardStop();
  tp->Wait();
}

TEST(Vector, MultiThreaded) {
  MultiThreaded<TestSuite::Vector>();
}

TEST(VoidVector, MultiThreaded) {
  MultiThreaded<TestSuite::Vector, void>();
}

TEST(Array, MultiThreaded) {
  MultiThreaded<TestSuite::Array>();
}

TEST(VoidArray, MultiThreaded) {
  MultiThreaded<TestSuite::Array, void>();
}

}  // namespace
