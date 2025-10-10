#include <util/error_code.hpp>

#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/make.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/async/shared_contract.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>
#include <yaclib/async/when_all.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
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

struct Int {};
struct Void {};

template <typename T>
class WhenAllT : public testing::Test {
 public:
  static constexpr bool kIsInt = std::is_same_v<Int, T>;
  static constexpr bool kIsVoid = std::is_same_v<Void, T>;
  static constexpr bool kIsAll = kIsInt || kIsVoid;
  static constexpr yaclib::FailPolicy Policy = kIsAll ? yaclib::FailPolicy::None : yaclib::FailPolicy::FirstFail;
  using Type = std::conditional_t<kIsInt, int, std::conditional_t<kIsVoid, void, T>>;
};

using MyTypes = ::testing::Types<int, void, Int, Void>;

struct TypeNames {
  template <typename T>
  static std::string GetName(int i) {
    switch (i) {
      case 0:
        return "int";
      case 1:
        return "void";
      case 2:
        return "int-all";
      case 3:
        return "void-all";
      default:
        return "unknown";
    }
  }
};

TYPED_TEST_SUITE(WhenAllT, MyTypes, TypeNames);

enum class FutureType {
  Future,
  SharedFuture,
  Mixed,
};

template <FutureType Type, typename V, typename E = yaclib::StopError>
struct ContractHelper;

template <typename V, typename E>
struct ContractHelper<FutureType::Future, V, E> {
  using FutureT = yaclib::Future<V, E>;
  using PromiseT = yaclib::Promise<V, E>;
  static auto MakeContract() {
    return yaclib::MakeContract<V, E>();
  }
};

template <typename V, typename E>
struct ContractHelper<FutureType::SharedFuture, V, E> {
  using FutureT = yaclib::SharedFuture<V, E>;
  using PromiseT = yaclib::SharedPromise<V, E>;
  static auto MakeContract() {
    return yaclib::MakeSharedContract<V, E>();
  }
};

template <FutureType Type, TestSuite suite, typename T, yaclib::FailPolicy F>
void JustWorks() {
  static_assert(!(Type == FutureType::Mixed && suite == TestSuite::Vector),
                "Mixed type only supports Array suite (fixed size arguments)");

  constexpr int kSize = 3;
  static constexpr bool is_void = std::is_void_v<T>;

  if constexpr (Type == FutureType::Mixed) {
    auto [future1, promise1] = yaclib::MakeContract<T>();
    auto [shared_future2, shared_promise2] = yaclib::MakeSharedContract<T>();
    auto [future3, promise3] = yaclib::MakeContract<T>();

    auto all = yaclib::WhenAll<F>(std::move(future1), std::move(shared_future2), std::move(future3));

    EXPECT_FALSE(all.Ready());

    if constexpr (is_void) {
      std::move(shared_promise2).Set();
      std::move(promise1).Set();
    } else {
      std::move(shared_promise2).Set(5);
      std::move(promise1).Set(3);
    }

    // Still not completed
    EXPECT_FALSE(all.Ready());

    if constexpr (is_void) {
      std::move(promise3).Set();
    } else {
      std::move(promise3).Set(7);
    }

    EXPECT_TRUE(all.Ready());
    const std::vector expected{3, 5, 7};

    if constexpr (F == yaclib::FailPolicy::None) {
      auto values = std::move(all).Touch().Value();
      std::size_t i = 0;
      for (const auto& v : values) {
        if constexpr (is_void) {
          EXPECT_EQ(v, yaclib::Unit{});
        } else {
          EXPECT_EQ(v, expected[i]);
        }
        ++i;
      }
      EXPECT_EQ(i, 3);
    } else if constexpr (is_void) {
      EXPECT_EQ(std::move(all).Get().State(), yaclib::ResultState::Value);
    } else {
      EXPECT_EQ(std::move(all).Get().Ok(), expected);
    }
  } else {
    using Helper = ContractHelper<Type, T>;
    std::array<typename Helper::PromiseT, kSize> promises;
    std::array<typename Helper::FutureT, kSize> futures;

    for (int i = 0; i < kSize; ++i) {
      std::tie(futures[i], promises[i]) = Helper::MakeContract();
    }

    auto all = [&futures] {
      if constexpr (suite == TestSuite::Array) {
        return yaclib::WhenAll<F>(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
      } else {
        return yaclib::WhenAll<F>(futures.begin(), futures.end());
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
    const std::vector expected{3, 5, 7};

    if constexpr (F == yaclib::FailPolicy::None) {
      auto values = std::move(all).Touch().Value();
      std::size_t i = 0;
      for (const auto& v : values) {
        if constexpr (is_void) {
          EXPECT_EQ(v, yaclib::Unit{});
        } else {
          EXPECT_EQ(v, expected[i]);
        }
        ++i;
      }
      EXPECT_EQ(i, 3);
    } else if constexpr (is_void) {
      EXPECT_EQ(std::move(all).Get().State(), yaclib::ResultState::Value);
    } else {
      EXPECT_EQ(std::move(all).Get().Ok(), expected);
    }
  }
}

TYPED_TEST(WhenAllT, VectorJustWorks) {
  JustWorks<FutureType::Future, TestSuite::Vector, typename TestFixture::Type, TestFixture::Policy>();
}

TYPED_TEST(WhenAllT, ArrayJustWorks) {
  JustWorks<FutureType::Future, TestSuite::Array, typename TestFixture::Type, TestFixture::Policy>();
}

TYPED_TEST(WhenAllT, SharedFutureVectorJustWorks) {
  JustWorks<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type, TestFixture::Policy>();
}

TYPED_TEST(WhenAllT, SharedFutureArrayJustWorks) {
  JustWorks<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type, TestFixture::Policy>();
}

TYPED_TEST(WhenAllT, MixedFutureSharedFutureJustWorks) {
  JustWorks<FutureType::Mixed, TestSuite::Array, typename TestFixture::Type, TestFixture::Policy>();
}

template <FutureType Type, TestSuite suite, typename T, typename E = yaclib::StopError>
void AllFails() {
  static_assert(!(Type == FutureType::Mixed && suite == TestSuite::Vector),
                "Mixed type only supports Array suite (fixed size arguments)");

  if constexpr (Type == FutureType::Mixed) {
    auto [future1, promise1] = yaclib::MakeContract<T, E>();
    auto [shared_future2, shared_promise2] = yaclib::MakeSharedContract<T, E>();
    auto [future3, promise3] = yaclib::MakeContract<T, E>();

    auto all = yaclib::WhenAll(std::move(future1), std::move(shared_future2), std::move(future3));

    EXPECT_FALSE(all.Ready());
    std::move(shared_promise2).Set(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_TRUE(all.Ready());

    // Second error
    std::move(promise1).Set(yaclib::StopTag{});
    EXPECT_THROW(std::ignore = std::move(all).Get().Ok(), std::runtime_error);
  } else {
    constexpr int kSize = 3;
    using Helper = ContractHelper<Type, T, E>;
    std::array<typename Helper::PromiseT, kSize> promises;
    std::array<typename Helper::FutureT, kSize> futures;

    for (int i = 0; i < kSize; ++i) {
      std::tie(futures[i], promises[i]) = Helper::MakeContract();
    }

    auto all = [&futures] {
      if constexpr (suite == TestSuite::Array) {
        return yaclib::WhenAll(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
      } else {
        return yaclib::WhenAll(futures.begin(), futures.end());
      }
    }();

    EXPECT_FALSE(all.Ready());
    std::move(promises[1]).Set(std::make_exception_ptr(std::runtime_error{""}));
    EXPECT_TRUE(all.Ready());

    // Second error
    std::move(promises[0]).Set(yaclib::StopTag{});
    EXPECT_THROW(std::ignore = std::move(all).Get().Ok(), std::runtime_error);
  }
}

TYPED_TEST(WhenAllT, FutureVectorAllFails) {
  AllFails<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type>();
  AllFails<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type, LikeErrorCode>();
  AllFails<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type, yaclib::StopError>();
  AllFails<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type, LikeErrorCode>();
}

TYPED_TEST(WhenAllT, FutureArrayAllFails) {
  AllFails<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type>();
  AllFails<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type, LikeErrorCode>();
  AllFails<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type, yaclib::StopError>();
  AllFails<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type, LikeErrorCode>();
}

TYPED_TEST(WhenAllT, SharedFutureVectorAllFails) {
  AllFails<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type>();
  AllFails<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type, LikeErrorCode>();
  AllFails<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type, yaclib::StopError>();
  AllFails<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type, LikeErrorCode>();
}

TYPED_TEST(WhenAllT, SharedFutureArrayAllFails) {
  AllFails<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type>();
  AllFails<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type, LikeErrorCode>();
  AllFails<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type, yaclib::StopError>();
  AllFails<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type, LikeErrorCode>();
}

TYPED_TEST(WhenAllT, MixedFutureSharedFutureAllFails) {
  AllFails<FutureType::Mixed, TestSuite::Array, typename TestFixture::Type>();
  AllFails<FutureType::Mixed, TestSuite::Array, typename TestFixture::Type, LikeErrorCode>();
}

template <FutureType Type, TestSuite suite, typename T>
void MultiThreaded() {
  static_assert(!(Type == FutureType::Mixed && suite == TestSuite::Vector),
                "Mixed type only supports Array suite (fixed size arguments)");
  static constexpr bool kIsVoid = std::is_void_v<T>;
  static constexpr int kValues = 6;

  yaclib::FairThreadPool tp{kValues};

  if constexpr (Type == FutureType::Mixed) {
    auto async_value = [&tp](int value) {
      return yaclib::Run(tp, [value] {
        yaclib_std::this_thread::sleep_for((kValues - value) * 10ms);
        if constexpr (kIsVoid) {
          std::ignore = value;
        } else {
          return value;
        }
      });
    };

    auto async_shared_value = [&tp](int value) {
      return yaclib::RunShared(tp, [value] {
        yaclib_std::this_thread::sleep_for((kValues - value) * 10ms);
        if constexpr (kIsVoid) {
          std::ignore = value;
        } else {
          return value;
        }
      });
    };

    auto f1 = async_value(0);
    auto sf2 = async_shared_value(1);
    auto f3 = async_value(2);
    auto sf4 = async_shared_value(3);
    auto f5 = async_value(4);
    auto sf6 = async_shared_value(5);

    auto ints =
      yaclib::WhenAll(std::move(f1), std::move(sf2), std::move(f3), std::move(sf4), std::move(f5), std::move(sf6))
        .Get();

    auto result = std::move(ints).Ok();
    if constexpr (!kIsVoid) {
      std::sort(result.begin(), result.end());
      EXPECT_EQ(result.size(), kValues);
      for (int i = 0; i < kValues; ++i) {
        EXPECT_EQ(result[i], i);
      }
    }
  } else {
    auto async_value = [&tp](int value) {
      if constexpr (Type == FutureType::SharedFuture) {
        return yaclib::RunShared(tp, [value] {
          yaclib_std::this_thread::sleep_for((kValues - value) * 10ms);
          if constexpr (kIsVoid) {
            std::ignore = value;
          } else {
            return value;
          }
        });
      } else {
        return yaclib::Run(tp, [value] {
          yaclib_std::this_thread::sleep_for((kValues - value) * 10ms);
          if constexpr (kIsVoid) {
            std::ignore = value;
          } else {
            return value;
          }
        });
      }
    };

    std::array<decltype(async_value(0)), kValues> fs;
    for (int i = 0; i < kValues; ++i) {
      fs[i] = async_value(i);
    }

    auto ints =
      [&fs] {
        if constexpr (suite == TestSuite::Vector) {
          return yaclib::WhenAll(fs.begin(), fs.end());
        } else {
          return yaclib::WhenAll(std::move(fs[0]), std::move(fs[1]), std::move(fs[2]), std::move(fs[3]),
                                 std::move(fs[4]), std::move(fs[5]));
        }
      }()
        .Get();

    auto result = std::move(ints).Ok();
    if constexpr (!kIsVoid) {
      EXPECT_EQ(result.size(), kValues);
      for (int i = 0; i < kValues; ++i) {
        EXPECT_EQ(result[i], i);
      }
    }
  }

  tp.HardStop();
  tp.Wait();
}

TYPED_TEST(WhenAllT, FutureVectorMultiThreaded) {
  MultiThreaded<FutureType::Future, TestSuite::Vector, typename TestFixture::Type>();
  MultiThreaded<FutureType::Future, TestSuite::Vector, typename TestFixture::Type>();
}

TYPED_TEST(WhenAllT, FutureArrayMultiThreaded) {
  MultiThreaded<FutureType::Future, TestSuite::Array, typename TestFixture::Type>();
  MultiThreaded<FutureType::Future, TestSuite::Array, typename TestFixture::Type>();
}

TYPED_TEST(WhenAllT, SharedFutureVectorMultiThreaded) {
  MultiThreaded<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type>();
  MultiThreaded<FutureType::SharedFuture, TestSuite::Vector, typename TestFixture::Type>();
}

TYPED_TEST(WhenAllT, SharedFutureArrayMultiThreaded) {
  MultiThreaded<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type>();
  MultiThreaded<FutureType::SharedFuture, TestSuite::Array, typename TestFixture::Type>();
}

TYPED_TEST(WhenAllT, MixedFutureSharedFutureMultiThreaded) {
  MultiThreaded<FutureType::Mixed, TestSuite::Array, typename TestFixture::Type>();
  MultiThreaded<FutureType::Mixed, TestSuite::Array, typename TestFixture::Type>();
}

template <TestSuite suite, typename T, typename E = yaclib::StopError>
void ErrorFails() {
  constexpr int kSize = 3;
  std::array<yaclib::Promise<T, E>, kSize> promises;
  std::array<yaclib::Future<T, E>, kSize> futures;
  for (int i = 0; i < kSize; ++i) {
    std::tie(futures[i], promises[i]) = yaclib::MakeContract<T, E>();
  }

  auto all = [&futures] {
    if constexpr (suite == TestSuite::Array) {
      return yaclib::WhenAll(std::move(futures[0]), std::move(futures[1]), std::move(futures[2]));
    } else {
      return yaclib::WhenAll(futures.begin(), futures.end());
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
  ErrorFails<TestSuite::Vector, typename TestFixture::Type, yaclib::StopError>();
  ErrorFails<TestSuite::Vector, typename TestFixture::Type, LikeErrorCode>();
}

TYPED_TEST(WhenAllT, ArrayErrorFails) {
  ErrorFails<TestSuite::Array, typename TestFixture::Type>();
  ErrorFails<TestSuite::Array, typename TestFixture::Type, LikeErrorCode>();
  ErrorFails<TestSuite::Array, typename TestFixture::Type, yaclib::StopError>();
  ErrorFails<TestSuite::Array, typename TestFixture::Type, LikeErrorCode>();
}

template <typename T>
void EmptyInput() {
  auto empty = std::vector<yaclib::Future<T>>{};
  auto all = yaclib::WhenAll(empty.begin(), empty.end());

  EXPECT_FALSE(all.Valid());
}

TEST(Vector, EmptyInput) {
  EmptyInput<int>();
  EmptyInput<void>();
}

template <typename Error = yaclib::StopError>
void FirstFail() {
  yaclib::FairThreadPool tp;
  std::vector<yaclib::FutureOn<void, Error>> ints;
  std::size_t count = yaclib_std::thread::hardware_concurrency() * 4;
  ints.reserve(count * 2);
  for (int j = 0; j != 200; ++j) {
    for (std::size_t i = 0; i != count; ++i) {
      ints.push_back(yaclib::Run<Error>(tp, [] {
        std::this_thread::sleep_for(4ms);
      }));
    }
    for (std::size_t i = 0; i != count; ++i) {
      ints.push_back(yaclib::Run<Error>(tp, [] {
        std::this_thread::sleep_for(2ms);
        return yaclib::Result<void, Error>{yaclib::StopTag{}};
      }));
    }
    EXPECT_THROW(std::ignore = WhenAll(ints.begin(), ints.end()).Get().Ok(), yaclib::ResultError<Error>);
    ints.clear();
  }
  tp.Stop();
  tp.Wait();
}

struct DeleteAssign {
  DeleteAssign() = default;
  DeleteAssign(DeleteAssign&&) = default;
  DeleteAssign(const DeleteAssign&) = default;
  DeleteAssign& operator=(DeleteAssign&&) = delete;
  DeleteAssign& operator=(const DeleteAssign&) = delete;
};

struct DeleteMove {
  DeleteMove() = default;
  DeleteMove(DeleteMove&&) = default;
  DeleteMove(const DeleteMove&) = default;
  DeleteMove& operator=(DeleteMove&&) = delete;
  DeleteMove& operator=(const DeleteMove&) = default;
};

struct DeleteCopy {
  DeleteCopy() = default;
  DeleteCopy(DeleteCopy&&) = default;
  DeleteCopy(const DeleteCopy&) = default;
  DeleteCopy& operator=(DeleteCopy&&) = default;
  DeleteCopy& operator=(const DeleteCopy&) = delete;
};

template <typename T>
void TestBadTypes() {
  auto f1 = yaclib::MakeFuture<T>();
  auto f2 = yaclib::MakeFuture<T>();
  auto f_all = yaclib::WhenAll<yaclib::FailPolicy::None>(std::move(f1), std::move(f2)).Get();
  EXPECT_EQ(f_all.State(), yaclib::ResultState::Value);
}

TEST(WhenAll, BadTypes) {
  TestBadTypes<DeleteCopy>();
  TestBadTypes<DeleteMove>();
  TestBadTypes<DeleteAssign>();
}

TEST(WhenAll, Bool) {
  yaclib::FairThreadPool tp{2};
  auto func = [] {
    yaclib_std::this_thread::sleep_for(100ms);
    return false;
  };
  auto f1 = yaclib::Run(tp, func);
  auto f2 = yaclib::Run(tp, func);
  auto v = yaclib::WhenAll(std::move(f1), std::move(f2)).Get().Ok();
  std::vector<bool> expected{false, false};
  EXPECT_EQ(v, expected);

  tp.Stop();
  tp.Wait();
}

TEST(WhenAll, FirstFail) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // Too long
#endif
  FirstFail();
  FirstFail<LikeErrorCode>();
}

}  // namespace
}  // namespace test
