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

enum CombinatorType {
  Static,
  Dynamic,
};

enum FutureType {
  Future,
  SharedFuture,
  Mixed,
};

template <std::size_t Index, FutureType Type, typename V, typename E = yaclib::StopError>
auto GetContract() {
  if constexpr (Type == Future || (Type == Mixed && Index % 2 == 0)) {
    return yaclib::MakeContract<V, E>();
  } else {
    return yaclib::MakeSharedContract<V, E>();
  }
}

template <std::size_t Index, std::size_t Limit, FutureType Type, typename V, typename E = yaclib::StopError>
auto GetAsyncValue(yaclib::IExecutor& e) {
  if constexpr (Type == Future || (Type == Mixed && Index % 2 == 0)) {
    return yaclib::Run(e, [] {
      yaclib_std::this_thread::sleep_for((Limit - Index) * 10ms);
      if constexpr (!std::is_void_v<V>) {
        return V{Index};
      }
    });
  } else {
    return yaclib::RunShared(e, [] {
      yaclib_std::this_thread::sleep_for((Limit - Index) * 10ms);
      if constexpr (!std::is_void_v<V>) {
        return V{Index};
      }
    });
  }
}

template <CombinatorType C, FutureType FT, yaclib::FailPolicy F, typename... Futures>
auto Combine(Futures... futures) {
  static_assert(C != Dynamic || FT != Mixed);

  if constexpr (C == Static) {
    return yaclib::WhenAll<F>(std::move(futures)...);
  } else {
    std::array<yaclib::head_t<Futures...>, sizeof...(Futures)> vec{std::move(futures)...};
    return yaclib::WhenAll<F>(vec.begin(), vec.end());
  }
}

template <CombinatorType _C, FutureType _FT, yaclib::FailPolicy _F, typename _T>
struct WhenAllTag {
  static constexpr CombinatorType C = _C;
  static constexpr FutureType FT = _FT;
  static constexpr yaclib::FailPolicy F = _F;
  using T = _T;
};

template <typename T>
struct WhenAllSuite : testing::Test {
  using Tag = T;
};

using WhenAllTypes = ::testing::Types<WhenAllTag<Static, Future, yaclib::FailPolicy::None, void>,
                                      WhenAllTag<Dynamic, Future, yaclib::FailPolicy::None, void>,

                                      WhenAllTag<Static, SharedFuture, yaclib::FailPolicy::None, void>,
                                      WhenAllTag<Dynamic, SharedFuture, yaclib::FailPolicy::None, void>,

                                      WhenAllTag<Static, Mixed, yaclib::FailPolicy::None, void>,

                                      WhenAllTag<Static, Future, yaclib::FailPolicy::FirstFail, void>,
                                      WhenAllTag<Dynamic, Future, yaclib::FailPolicy::FirstFail, void>,

                                      WhenAllTag<Static, SharedFuture, yaclib::FailPolicy::FirstFail, void>,
                                      WhenAllTag<Dynamic, SharedFuture, yaclib::FailPolicy::FirstFail, void>,

                                      WhenAllTag<Static, Mixed, yaclib::FailPolicy::FirstFail, void>,

                                      WhenAllTag<Static, Future, yaclib::FailPolicy::None, int>,
                                      WhenAllTag<Dynamic, Future, yaclib::FailPolicy::None, int>,

                                      WhenAllTag<Static, SharedFuture, yaclib::FailPolicy::None, int>,
                                      WhenAllTag<Dynamic, SharedFuture, yaclib::FailPolicy::None, int>,

                                      WhenAllTag<Static, Mixed, yaclib::FailPolicy::None, int>,

                                      WhenAllTag<Static, Future, yaclib::FailPolicy::FirstFail, int>,
                                      WhenAllTag<Dynamic, Future, yaclib::FailPolicy::FirstFail, int>,

                                      WhenAllTag<Static, SharedFuture, yaclib::FailPolicy::FirstFail, int>,
                                      WhenAllTag<Dynamic, SharedFuture, yaclib::FailPolicy::FirstFail, int>,

                                      WhenAllTag<Static, Mixed, yaclib::FailPolicy::FirstFail, int>>;

struct WhenAllNames {
  template <typename T>
  static std::string GetName(int i) {
    std::string result;
    result += (T::C == Static ? "Static_" : "Dynamic_");
    result += (T::FT == Future ? "Future_" : (T::FT == SharedFuture ? "SharedFuture_" : "Mixed_"));
    result += (T::F == yaclib::FailPolicy::None ? "None_" : "FirstFail_");
    result += (std::is_same_v<typename T::T, void> ? "void" : "int");
    return result;
  }
};

TYPED_TEST_SUITE(WhenAllSuite, WhenAllTypes, WhenAllNames);

TYPED_TEST(WhenAllSuite, JustWorks) {
  using T = typename TestFixture::Tag;
  static constexpr bool is_void = std::is_void_v<typename T::T>;

  auto [f1, p1] = GetContract<0, T::FT, typename T::T>();
  auto [f2, p2] = GetContract<1, T::FT, typename T::T>();
  auto [f3, p3] = GetContract<2, T::FT, typename T::T>();

  auto all = Combine<T::C, T::FT, T::F>(std::move(f1), std::move(f2), std::move(f3));

  EXPECT_FALSE(all.Ready());

  if constexpr (is_void) {
    std::move(p2).Set();
    std::move(p1).Set();
  } else {
    std::move(p2).Set(5);
    std::move(p1).Set(3);
  }

  // Still not completed
  EXPECT_FALSE(all.Ready());

  if constexpr (is_void) {
    std::move(p3).Set();
  } else {
    std::move(p3).Set(7);
  }

  EXPECT_TRUE(all.Ready());
  const std::vector expected{3, 5, 7};

  if constexpr (T::F == yaclib::FailPolicy::None) {
    auto values = std::move(all).Touch().Value();
    std::size_t i = 0;
    for (const auto& v : values) {
      if constexpr (is_void) {
        EXPECT_EQ(v.Ok(), yaclib::Unit{});
      } else {
        EXPECT_EQ(v.Ok(), expected[i]);
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

TYPED_TEST(WhenAllSuite, AllFails) {
  using T = typename TestFixture::Tag;
  static constexpr bool is_void = std::is_void_v<typename T::T>;

  auto [f1, p1] = GetContract<0, T::FT, typename T::T>();
  auto [f2, p2] = GetContract<1, T::FT, typename T::T>();
  auto [f3, p3] = GetContract<2, T::FT, typename T::T>();

  auto all = Combine<T::C, T::FT, T::F>(std::move(f1), std::move(f2), std::move(f3));

  EXPECT_FALSE(all.Ready());
  std::move(p1).Set(std::make_exception_ptr(std::runtime_error{""}));

  if constexpr (T::F == yaclib::FailPolicy::FirstFail) {
    EXPECT_TRUE(all.Ready());
  }

  std::move(p2).Set(yaclib::StopTag{});
  std::move(p3).Set(yaclib::StopTag{});

  EXPECT_TRUE(all.Ready());

  if constexpr (T::F == yaclib::FailPolicy::FirstFail) {
    EXPECT_THROW(std::ignore = std::move(all).Get().Ok(), std::runtime_error);
  } else {
    auto values = std::move(all).Touch().Value();
    EXPECT_EQ(values.size(), 3);
    EXPECT_EQ(values[0].State(), yaclib::ResultState::Exception);
    EXPECT_EQ(values[1].State(), yaclib::ResultState::Error);
    EXPECT_EQ(values[2].State(), yaclib::ResultState::Error);
  }
}

TYPED_TEST(WhenAllSuite, MultiThreaded) {
  using T = typename TestFixture::Tag;
  static constexpr bool is_void = std::is_void_v<typename T::T>;
  static constexpr int kValues = 6;

  yaclib::FairThreadPool tp{kValues};

  auto f1 = GetAsyncValue<0, kValues, T::FT, typename T::T>(tp);
  auto f2 = GetAsyncValue<1, kValues, T::FT, typename T::T>(tp);
  auto f3 = GetAsyncValue<2, kValues, T::FT, typename T::T>(tp);
  auto f4 = GetAsyncValue<3, kValues, T::FT, typename T::T>(tp);
  auto f5 = GetAsyncValue<4, kValues, T::FT, typename T::T>(tp);
  auto f6 = GetAsyncValue<5, kValues, T::FT, typename T::T>(tp);

  auto all = Combine<T::C, T::FT, T::F>(std::move(f1), std::move(f2), std::move(f3), std::move(f4), std::move(f5),
                                        std::move(f6));

  auto result = std::move(all).Get().Ok();

  if constexpr (is_void) {
    if constexpr (T::F == yaclib::FailPolicy::None) {
      for (size_t i = 0; i < kValues; ++i) {
        EXPECT_EQ(std::move(result[i]).Value(), yaclib::Unit{});
      }
    } else {
      EXPECT_EQ(result, yaclib::Unit{});
    }
  } else {
    for (size_t i = 0; i < kValues; ++i) {
      if constexpr (T::F == yaclib::FailPolicy::None) {
        EXPECT_EQ(std::move(result[i]).Value(), i);
      } else {
        EXPECT_EQ(result[i], i);
      }
    }
  }

  tp.HardStop();
  tp.Wait();
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

TEST(WhenAll, FirstFail) {
#if YACLIB_FAULT == 2
  GTEST_SKIP();  // Too long
#endif
  FirstFail();
  FirstFail<LikeErrorCode>();
}

TEST(WhenAll, FailWithError) {
  auto f1 = yaclib::MakeFuture<void>(yaclib::StopTag{});
  auto f2 = yaclib::MakeFuture<void>(yaclib::Unit{});
  auto all1 = yaclib::WhenAll(std::move(f1), std::move(f2)).Get();
  EXPECT_EQ(std::move(all1).Error(), yaclib::StopTag{});

  auto f3 = yaclib::MakeFuture<int>(yaclib::StopTag{});
  auto f4 = yaclib::MakeFuture<int>(3);
  auto all2 = yaclib::WhenAll(std::move(f1), std::move(f2)).Get();
  EXPECT_EQ(std::move(all2).Error(), yaclib::StopTag{});
}

}  // namespace
}  // namespace test
