#include <yaclib/algo/when_all.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <iostream>
#include <string>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;
using namespace std::chrono_literals;

enum class TestSuite {
  Vector,
  Array,
};

template <typename T>
class WaitAllT : public testing::Test {
 public:
  using Type = T;
};

using MyTypes = ::testing::Types<int, void>;

TYPED_TEST_SUITE(WaitAllT, MyTypes);

template <TestSuite suite, typename T = int>
void JustWorks() {
  constexpr int kSize = 3;
  static constexpr bool is_void = std::is_void_v<T>;

  std::array<Promise<T>, kSize> promises;
  std::array<Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = MakeContract<T>();
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
      return std::array{7, 3, 5};
    }
  }();
  if constexpr (is_void) {
    EXPECT_EQ(std::move(all).Get().State(), util::ResultState::Value);
  } else {
    EXPECT_EQ(std::move(all).Get().Ok(), expected);
  }
}

TYPED_TEST(WaitAllT, VectorJustWorks) {
  JustWorks<TestSuite::Vector, typename TestFixture::Type>();
}

TYPED_TEST(WaitAllT, ArrayJustWorks) {
  JustWorks<TestSuite::Array, typename TestFixture::Type>();
}

template <TestSuite suite, typename T = void>
void AllFails() {
  constexpr int kSize = 3;
  std::array<Promise<T>, kSize> promises;
  std::array<Future<T>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = MakeContract<T>();
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
  std::move(promises[0]).Set(std::error_code{});

  EXPECT_THROW(std::move(all).Get().Ok(), std::runtime_error);
}

TYPED_TEST(WaitAllT, VectorAllFails) {
  AllFails<TestSuite::Vector, typename TestFixture::Type>();
}

TYPED_TEST(WaitAllT, ArrayAllFails) {
  AllFails<TestSuite::Array, typename TestFixture::Type>();
}

template <typename T = int>
void EmptyInput() {
  auto empty = std::vector<Future<T>>{};
  auto all = WhenAll(empty.begin(), empty.end());

  EXPECT_TRUE(all.Ready());
  EXPECT_NO_THROW(std::move(all).Get().Ok());
}

TEST(Vector, EmptyInput) {
  EmptyInput();
}

TEST(VoidVector, EmptyInput) {
  EmptyInput<void>();
}

template <TestSuite suite, typename T = int>
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

  std::array<Future<T>, 6> fs;
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

  if constexpr (is_void) {
    EXPECT_NO_THROW(std::move(ints).Ok());
  } else {
    auto result = std::move(ints).Ok();
    std::sort(result.begin(), result.end());
    EXPECT_EQ(result.size(), kValues);
    for (int i = 0; i < (int)kValues; ++i) {
      EXPECT_EQ(result[i], i);
    }
  }

  tp->HardStop();
  tp->Wait();
}

TYPED_TEST(WaitAllT, VectorMultiThreaded) {
  MultiThreaded<TestSuite::Vector, typename TestFixture::Type>();
}

TYPED_TEST(WaitAllT, ArrayMultiThreaded) {
  MultiThreaded<TestSuite::Array, typename TestFixture::Type>();
}

}  // namespace
