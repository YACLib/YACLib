#include <util/error_code.hpp>

#include <yaclib/algo/when_all.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <exception>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

enum class TestSuite {
  Vector,
  Array,
};

template <typename T>
class WhenAllT : public testing::Test {
 public:
  using Type = T;
};

using MyTypes = ::testing::Types<int, void>;

TYPED_TEST_SUITE(WhenAllT, MyTypes);

template <TestSuite suite, typename T = int>
void JustWorks() {
  constexpr int kSize = 3;
  static constexpr bool is_void = std::is_void_v<T>;

  std::array<yaclib::Promise<T>, kSize> promises;
  std::array<yaclib::Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    std::tie(futures[i], promises[i]) = yaclib::MakeContract<T>();
  }

  auto all = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return WhenAll(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return WhenAll(futures.begin(), futures.end());
    }
  }();

  EXPECT_FALSE(all.Ready());

  if constexpr (is_void) {
    std::move(promises[2]).Set();
    std::move(promises[0]).Set();
  } else {
    std::move(promises[2]).Set(7);
    std::move(promises[0]).Set(3);
  }

  // Still not completed
  EXPECT_FALSE(all.Ready());

  if constexpr (is_void) {
    std::move(promises[1]).Set();
  } else {
    std::move(promises[1]).Set(5);
  }

  EXPECT_TRUE(all.Ready());

  auto expected = [] {
    if constexpr (suite == TestSuite::Vector) {
      return std::vector{7, 3, 5};
    } else {
      return std::array<int, 3>{7, 3, 5};
    }
  }();
  if constexpr (is_void) {
    EXPECT_EQ(std::move(all).Get().State(), yaclib::ResultState::Value);
  } else {
    EXPECT_EQ(std::move(all).Get().Ok(), expected);
  }
}

TYPED_TEST(WhenAllT, VectorJustWorks) {
  JustWorks<TestSuite::Vector, typename TestFixture::Type>();
}

TYPED_TEST(WhenAllT, ArrayJustWorks) {
  JustWorks<TestSuite::Array, typename TestFixture::Type>();
}

template <TestSuite suite, typename T = void, typename E = yaclib::StopError>
void AllFails() {
  constexpr int kSize = 3;
  std::array<yaclib::Promise<T, E>, kSize> promises;
  std::array<yaclib::Future<T, E>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = yaclib::MakeContract<T, E>();
    futures[i] = std::move(f);
    promises[i] = std::move(p);
  }

  auto all = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return WhenAll(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return WhenAll(futures.begin(), futures.end());
    }
  }();

  EXPECT_FALSE(all.Ready());

  std::move(promises[1]).Set(std::make_exception_ptr(std::runtime_error{""}));

  EXPECT_TRUE(all.Ready());

  // Second error
  std::move(promises[0]).Set(yaclib::StopTag{});

  EXPECT_THROW(std::move(all).Get().Ok(), std::runtime_error);
}

TYPED_TEST(WhenAllT, VectorAllFails) {
  AllFails<TestSuite::Vector, typename TestFixture::Type>();
  AllFails<TestSuite::Vector, typename TestFixture::Type, LikeErrorCode>();
}

TYPED_TEST(WhenAllT, ArrayAllFails) {
  AllFails<TestSuite::Array, typename TestFixture::Type>();
  AllFails<TestSuite::Array, typename TestFixture::Type, LikeErrorCode>();
}

template <TestSuite suite, typename T = void, typename E = yaclib::StopError>
void ErrorFails() {
  constexpr int kSize = 3;
  std::array<yaclib::Promise<T, E>, kSize> promises;
  std::array<yaclib::Future<T, E>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    std::tie(futures[i], promises[i]) = yaclib::MakeContract<T, E>();
  }

  auto all = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return WhenAll(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return WhenAll(futures.begin(), futures.end());
    }
  }();

  EXPECT_FALSE(all.Ready());

  std::move(promises[1]).Set(yaclib::StopTag{});
  EXPECT_TRUE(all.Ready());
  EXPECT_FALSE(promises[1].Valid());
}

TYPED_TEST(WhenAllT, VectorErrorFails) {
  ErrorFails<TestSuite::Vector, typename TestFixture::Type>();
  ErrorFails<TestSuite::Vector, typename TestFixture::Type, LikeErrorCode>();
}

TYPED_TEST(WhenAllT, ArrayErrorFails) {
  ErrorFails<TestSuite::Array, typename TestFixture::Type>();
  ErrorFails<TestSuite::Array, typename TestFixture::Type, LikeErrorCode>();
}

template <typename T = int>
void EmptyInput() {
  auto empty = std::vector<yaclib::Future<T>>{};
  auto all = WhenAll(empty.begin(), empty.end());

  EXPECT_FALSE(all.Valid());
}

TEST(Vector, EmptyInput) {
  EmptyInput();
}

TEST(VoidVector, EmptyInput) {
  EmptyInput<void>();
}

template <TestSuite suite, typename T = int>
void MultiThreaded() {
  static constexpr bool kIsVoid = std::is_void_v<T>;

  auto tp = yaclib::MakeThreadPool(4);

  auto async_value = [tp](int value) {
    return Run(*tp, [value] {
      yaclib_std::this_thread::sleep_for(10ms);
      if constexpr (kIsVoid) {
        std::ignore = value;
      } else {
        return value;
      }
    });
  };

  static const int kValues = 6;

  std::array<yaclib::FutureOn<T>, 6> fs;
  for (int i = 0; i < kValues; ++i) {
    fs[i] = async_value(i);
  }

  auto ints =
    [&fs] {
      if constexpr (suite == TestSuite::Vector) {
        return WhenAll(fs.begin(), fs.end());
      } else {
        return WhenAll(std::move(fs[0]), std::move(fs[1]), std::move(fs[2]), std::move(fs[3]), std::move(fs[4]),
                       std::move(fs[5]));
      }
    }()
      .Get();

  if constexpr (kIsVoid) {
    EXPECT_NO_THROW(std::move(ints).Ok());
  } else {
    auto result = std::move(ints).Ok();
    std::sort(result.begin(), result.end());
    EXPECT_EQ(result.size(), kValues);
    for (int i = 0; i < kValues; ++i) {
      EXPECT_EQ(result[i], i);
    }
  }

  tp->HardStop();
  tp->Wait();
}

TYPED_TEST(WhenAllT, VectorMultiThreaded) {
  MultiThreaded<TestSuite::Vector, typename TestFixture::Type>();
}

TYPED_TEST(WhenAllT, ArrayMultiThreaded) {
  MultiThreaded<TestSuite::Array, typename TestFixture::Type>();
}

template <typename Error = yaclib::StopError>
void FirstFail() {
  auto tp = yaclib::MakeThreadPool();
  std::vector<yaclib::FutureOn<void, Error>> ints;
  std::size_t count = std::thread::hardware_concurrency() * 4;
  ints.reserve(count * 2);
  for (int j = 0; j != 200; ++j) {
    for (std::size_t i = 0; i != count; ++i) {
      ints.push_back(yaclib::Run<Error>(*tp, [] {
        std::this_thread::sleep_for(4ms);
      }));
    }
    for (std::size_t i = 0; i != count; ++i) {
      ints.push_back(yaclib::Run<Error>(*tp, [] {
        std::this_thread::sleep_for(2ms);
        return yaclib::Result<void, Error>{Error{yaclib::StopTag{}}};
      }));
    }
    EXPECT_THROW(WhenAll(ints.begin(), ints.end()).Get().Ok(), yaclib::ResultError<Error>);
    ints.clear();
  }
  tp->Stop();
  tp->Wait();
}

TEST(WhenAll, FirstFail) {
  FirstFail();
  FirstFail<LikeErrorCode>();
}

}  // namespace
}  // namespace test
