#include <yaclib/algo/when_any.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <array>
#include <string>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;
using namespace std::chrono_literals;

enum class TestSuite { Vector, Array };

template <TestSuite suite, typename T = int, PolicyWhenAny P = PolicyWhenAny::FirstError>
void JustWorks() {
  constexpr int kSize = 3;

  std::array<Promise<T>, kSize> promises;
  std::array<Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = MakeContract<T>();
    futures[i] = std::move(f);
    promises[i] = std::move(p);
  }
  auto any = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return WhenAny<P>(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return WhenAny<P>(futures.begin(), futures.end());
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

TEST(VectorFirstError, JustWorks) {
  JustWorks<TestSuite::Vector>();
}

TEST(VectorLastError, JustWorks) {
  JustWorks<TestSuite::Vector, int, PolicyWhenAny::LastError>();
}

TEST(VoidVectorFirstError, JustWorks) {
  JustWorks<TestSuite::Vector, void>();
}

TEST(VoidVectorLastError, JustWorks) {
  JustWorks<TestSuite::Vector, void, PolicyWhenAny::LastError>();
}

TEST(ArrayFirstError, JustWorks) {
  JustWorks<TestSuite::Array>();
}

TEST(ArrayLastError, JustWorks) {
  JustWorks<TestSuite::Array, int, PolicyWhenAny::LastError>();
}

TEST(VoidArrayFirstError, JustWorks) {
  JustWorks<TestSuite::Array, void>();
}

TEST(VoidArrayLastError, JustWorks) {
  JustWorks<TestSuite::Array, void, PolicyWhenAny::LastError>();
}

template <TestSuite suite, typename T = void, PolicyWhenAny P = PolicyWhenAny::FirstError>
void AllFails() {
  constexpr int kSize = 3;
  std::array<Promise<T>, kSize> promises;
  std::array<Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = MakeContract<T>();
    futures[i] = std::move(f);
    promises[i] = std::move(p);
  }

  auto any = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return WhenAny<P>(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return WhenAny<P>(futures.begin(), futures.end());
    }
  }();

  EXPECT_FALSE(any.Ready());

  std::move(promises[0]).Set(std::make_exception_ptr(std::runtime_error{""}));

  EXPECT_FALSE(any.Ready());

  // Second and third error
  std::move(promises[1]).Set(std::error_code{});
  std::move(promises[2]).Set(std::error_code{});

  EXPECT_TRUE(any.Ready());
  if constexpr (P == PolicyWhenAny::FirstError) {
    EXPECT_THROW(std::move(any).Get().Ok(), std::runtime_error);
  } else {
    EXPECT_THROW(std::move(any).Get().Ok(), std::exception);
  }
}

TEST(VectorFirstError, AllFails) {
  AllFails<TestSuite::Vector>();
}

TEST(VectorLastError, AllFails) {
  AllFails<TestSuite::Vector, int, PolicyWhenAny::LastError>();
}

TEST(VoidVectorFirstError, AllFails) {
  AllFails<TestSuite::Vector, void>();
}

TEST(VoidVectorLastError, AllFails) {
  AllFails<TestSuite::Vector, void, PolicyWhenAny::LastError>();
}

TEST(ArrayFirstError, AllFails) {
  AllFails<TestSuite::Array>();
}

TEST(ArrayLastError, AllFails) {
  AllFails<TestSuite::Array, int, PolicyWhenAny::LastError>();
}

TEST(VoidArrayFirstError, AllFails) {
  AllFails<TestSuite::Array, void>();
}

TEST(VoidArrayLastError, AllFails) {
  AllFails<TestSuite::Array, void, PolicyWhenAny::LastError>();
}

template <TestSuite suite, typename T = int, PolicyWhenAny P = PolicyWhenAny::FirstError>
void ResultWithFails() {
  constexpr int kSize = 3;
  std::array<Promise<T>, kSize> promises;
  std::array<Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = MakeContract<T>();
    futures[i] = std::move(f);
    promises[i] = std::move(p);
  }

  auto any = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return WhenAny<P>(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return WhenAny<P>(futures.begin(), futures.end());
    }
  }();

  EXPECT_FALSE(any.Ready());

  std::move(promises[0]).Set(std::make_exception_ptr(std::runtime_error{""}));

  EXPECT_FALSE(any.Ready());

  if constexpr (std::is_void_v<T>) {
    std::move(promises[1]).Set();
  } else {
    std::move(promises[1]).Set(5);
  }

  EXPECT_TRUE(any.Ready());

  if constexpr (!std::is_void_v<T>) {
    EXPECT_EQ(std::move(any).Get().Ok(), 5);
  }
}

TEST(VectorFirstError, ResultWithFails) {
  ResultWithFails<TestSuite::Vector>();
}

TEST(VectorLastError, ResultWithFails) {
  ResultWithFails<TestSuite::Vector, int, PolicyWhenAny::LastError>();
}

TEST(VoidVectorFirstError, ResultWithFails) {
  ResultWithFails<TestSuite::Vector, void>();
}

TEST(VoidVectorLastError, ResultWithFails) {
  ResultWithFails<TestSuite::Vector, void, PolicyWhenAny::LastError>();
}

TEST(ArrayFirstError, ResultWithFails) {
  ResultWithFails<TestSuite::Array>();
}

TEST(ArrayLastError, ResultWithFails) {
  ResultWithFails<TestSuite::Array, int, PolicyWhenAny::LastError>();
}

TEST(VoidArrayFirstError, ResultWithFails) {
  ResultWithFails<TestSuite::Array, void>();
}

TEST(VoidArrayLastError, ResultWithFails) {
  ResultWithFails<TestSuite::Array, void, PolicyWhenAny::LastError>();
}

template <typename T = int, PolicyWhenAny P = PolicyWhenAny::FirstError>
void EmptyInput() {
  util::Result<T> res{};
  auto empty = std::vector<Future<T>>{};
  auto any = WhenAny<P>(empty.begin(), empty.end());

  EXPECT_TRUE(any.Ready());
  EXPECT_THROW(std::move(any).Get().Ok(), std::exception);
}

TEST(VectorFirstError, EmptyInput) {
  EmptyInput();
}

TEST(VectorLastError, EmptyInput) {
  EmptyInput<int, PolicyWhenAny::LastError>();
}

TEST(VoidVectorFirstError, EmptyInput) {
  EmptyInput<void>();
}

TEST(VoidVectorLastError, EmptyInput) {
  EmptyInput<void, PolicyWhenAny::LastError>();
}

template <TestSuite suite, typename T = int, PolicyWhenAny P = PolicyWhenAny::FirstError>
void MultiThreaded() {
  static constexpr bool is_void = std::is_void_v<T>;

  auto tp = MakeThreadPool(4);

  auto async_value = [tp](int value) {
    return Run(tp, [value] {
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

  std::array<Future<T>, 6> fs;
  for (int i = 0; i < kValues; ++i) {
    fs[i] = async_value(i);
  }

  auto ints =
      [&fs] {
        if constexpr (suite == TestSuite::Vector) {
          return WhenAny<P>(fs.begin(), fs.end());
        } else {
          return WhenAny<P>(std::move(fs[0]), std::move(fs[1]), std::move(fs[2]), std::move(fs[3]), std::move(fs[4]),
                            std::move(fs[5]));
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

TEST(VectorFirstError, MultiThreaded) {
  MultiThreaded<TestSuite::Vector>();
}

TEST(VectorLastError, MultiThreaded) {
  MultiThreaded<TestSuite::Vector, int, PolicyWhenAny::LastError>();
}

TEST(VoidVectorFirstError, MultiThreaded) {
  MultiThreaded<TestSuite::Vector, void>();
}

TEST(VoidVectorLastError, MultiThreaded) {
  MultiThreaded<TestSuite::Vector, void, PolicyWhenAny::LastError>();
}

TEST(ArrayFirstError, MultiThreaded) {
  MultiThreaded<TestSuite::Array>();
}

TEST(ArrayLastError, MultiThreaded) {
  MultiThreaded<TestSuite::Array, int, PolicyWhenAny::LastError>();
}

TEST(VoidArrayFirstError, MultiThreaded) {
  MultiThreaded<TestSuite::Array, void>();
}

TEST(VoidArrayLastError, MultiThreaded) {
  MultiThreaded<TestSuite::Array, void, PolicyWhenAny::LastError>();
}

template <TestSuite suite, typename T = int, PolicyWhenAny P = PolicyWhenAny::FirstError>
void TimeTest() {
  using Duration = std::chrono::nanoseconds;
  static constexpr bool is_void = std::is_void_v<T>;

  auto tp = MakeThreadPool(4);

  auto async_value = [tp](int value, Duration d) {
    return Run(tp, [value, d] {
      std::this_thread::sleep_for(d);
      if constexpr (!is_void) {
        return value;
      } else {
        (void)value;
      }
    });
  };

  static const int kValues = 2;

  std::array<Future<T>, kValues> fs;

  fs[0] = async_value(10, 200ms);
  fs[1] = async_value(5, 50ms);

  auto ints =
      [&fs] {
        if constexpr (suite == TestSuite::Vector) {
          return WhenAny<P>(fs.begin(), fs.end());
        } else {
          return WhenAny<P>(std::move(fs[0]), std::move(fs[1]));
        }
      }()
          .Get();

  if constexpr (is_void) {
    EXPECT_NO_THROW(std::move(ints).Ok());
  } else {
    auto result = std::move(ints).Ok();
    EXPECT_EQ(result, 5);
  }

  tp->HardStop();
  tp->Wait();
}

TEST(VectorFirstError, TimeTest) {
  TimeTest<TestSuite::Vector>();
}

TEST(VectorLastError, TimeTest) {
  TimeTest<TestSuite::Vector, int, PolicyWhenAny::LastError>();
}

TEST(VoidVectorFirstError, TimeTest) {
  TimeTest<TestSuite::Vector, void>();
}

TEST(VoidVectorLastError, TimeTest) {
  TimeTest<TestSuite::Vector, void, PolicyWhenAny::LastError>();
}

TEST(ArrayFirstError, TimeTest) {
  TimeTest<TestSuite::Array>();
}

TEST(ArrayLastError, TimeTest) {
  TimeTest<TestSuite::Array, int, PolicyWhenAny::LastError>();
}

TEST(VoidArrayFirstError, TimeTest) {
  TimeTest<TestSuite::Array, void>();
}

TEST(VoidArrayLastError, TimeTest) {
  TimeTest<TestSuite::Array, void, PolicyWhenAny::LastError>();
}

template <TestSuite suite, typename T = int>
void DefaultPolice() {
  constexpr int kSize = 3;

  std::array<Promise<T>, kSize> promises;
  std::array<Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = MakeContract<T>();
    futures[i] = std::move(f);
    promises[i] = std::move(p);
  }
  auto any = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return WhenAny(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return WhenAny(futures.begin(), futures.end());
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

TEST(VectorFirstError, DefaultPolice) {
  DefaultPolice<TestSuite::Vector>();
}

TEST(VoidVectorFirstError, DefaultPolice) {
  DefaultPolice<TestSuite::Vector, void>();
}

TEST(ArrayFirstError, DefaultPolice) {
  DefaultPolice<TestSuite::Array>();
}

TEST(VoidArrayFirstError, DefaultPolice) {
  DefaultPolice<TestSuite::Array, void>();
}

}  // namespace
