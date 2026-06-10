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
#include <memory>
#include <stdexcept>
#include <system_error>
#include <thread>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>
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

template <std::size_t Index, FutureType Type, typename V, typename T = yaclib::DefaultTrait>
auto GetContract() {
  if constexpr (Type == Future || (Type == Mixed && Index % 2 == 0)) {
    return yaclib::MakeContract<V, T>();
  } else {
    return yaclib::MakeSharedContract<V, T>();
  }
}

template <std::size_t Index, std::size_t Limit, FutureType Type, typename V, typename T = yaclib::DefaultTrait>
auto GetAsyncValue(yaclib::IExecutor& e) {
  if constexpr (Type == Future || (Type == Mixed && Index % 2 == 0)) {
    return yaclib::Run<T>(e, [] {
      yaclib_std::this_thread::sleep_for((Limit - Index) * 10ms);
      if constexpr (!std::is_void_v<V>) {
        return V{Index};
      }
    });
  } else {
    return yaclib::RunShared<T>(e, [] {
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
    EXPECT_TRUE(std::move(all).Get());
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
    // Exception and Error states are unified now: check error-ness and use IsStop to distinguish cancellation
    const auto& v0 = values[0];
    EXPECT_FALSE(v0);
    EXPECT_FALSE(yaclib::IsStop(v0.Error()));
    EXPECT_THROW(std::rethrow_exception(v0.Error()), std::runtime_error);
    const auto& v1 = values[1];
    EXPECT_FALSE(v1);
    EXPECT_TRUE(yaclib::IsStop(v1.Error()));
    const auto& v2 = values[2];
    EXPECT_FALSE(v2);
    EXPECT_TRUE(yaclib::IsStop(v2.Error()));
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

template <typename T = yaclib::DefaultTrait>
void FirstFail() {
  yaclib::FairThreadPool tp;
  std::vector<yaclib::FutureOn<void, T>> ints;
  std::size_t count = yaclib_std::thread::hardware_concurrency() * 4;
  ints.reserve(count * 2);
  for (int j = 0; j != 200; ++j) {
    for (std::size_t i = 0; i != count; ++i) {
      ints.push_back(yaclib::Run<T>(tp, [] {
        std::this_thread::sleep_for(4ms);
      }));
    }
    for (std::size_t i = 0; i != count; ++i) {
      ints.push_back(yaclib::Run<T>(tp, [] {
        std::this_thread::sleep_for(2ms);
        return T::template MakeResult<void>(yaclib::StopTag{});
      }));
    }
    if constexpr (std::is_same_v<T, yaclib::DefaultTrait>) {
      EXPECT_THROW(std::ignore = WhenAll(ints.begin(), ints.end()).Get().Ok(), yaclib::StopException);
    } else {
      EXPECT_THROW(std::ignore = WhenAll(ints.begin(), ints.end()).Get().Ok(), std::system_error);
    }
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
  EXPECT_TRUE(f_all);
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
  FirstFail<ErrorCodeTrait>();
}

/**
 * Error type with an observably destructive move: moving steals the shared_ptr, leaving the source empty.
 *
 * Used to check that All<FirstFail>::Consume copies (not moves) the error from a live shared input core.
 */
struct DestructiveError {
  DestructiveError() noexcept = default;

  explicit DestructiveError(std::error_code error) : code{std::make_shared<std::error_code>(error)} {
  }

  DestructiveError(yaclib::StopTag /*tag*/)
    : code{std::make_shared<std::error_code>(std::make_error_code(std::errc::operation_canceled))} {
  }

  DestructiveError(DestructiveError&&) noexcept = default;
  DestructiveError(const DestructiveError&) noexcept = default;
  DestructiveError& operator=(DestructiveError&&) noexcept = default;
  DestructiveError& operator=(const DestructiveError&) noexcept = default;

  std::shared_ptr<std::error_code> code;
};

/**
 * Minimal result container like test::Expected, but with DestructiveError errors and Error()&& that really moves
 */
template <typename ValueT>
class Destructive final {
  using V = std::conditional_t<std::is_void_v<ValueT>, yaclib::Unit, ValueT>;
  using Variant = std::variant<V, DestructiveError>;

 public:
  Destructive() : _result{std::in_place_index<1>, DestructiveError{yaclib::StopTag{}}} {
  }

  Destructive(yaclib::StopTag tag) : _result{std::in_place_index<1>, DestructiveError{tag}} {
  }

  Destructive(DestructiveError error) : _result{std::in_place_index<1>, std::move(error)} {
  }

  template <typename... Args>
  explicit Destructive(std::in_place_t, Args&&... args) : _result{std::in_place_index<0>, std::forward<Args>(args)...} {
  }

  [[nodiscard]] explicit operator bool() const noexcept {
    return _result.index() == 0;
  }

  [[nodiscard]] V&& Value() && noexcept {
    return std::move(*std::get_if<0>(&_result));
  }
  [[nodiscard]] const V& Value() const& noexcept {
    return *std::get_if<0>(&_result);
  }

  [[nodiscard]] DestructiveError&& Error() && noexcept {
    return std::move(*std::get_if<1>(&_result));
  }
  [[nodiscard]] const DestructiveError& Error() const& noexcept {
    return *std::get_if<1>(&_result);
  }

 private:
  Variant _result;
};

/**
 * Result trait that plugs \ref Destructive into yaclib, \see yaclib::ResultTrait for the contract
 */
struct DestructiveTrait {
  template <typename V>
  using Result = Destructive<V>;

  using Error = DestructiveError;

  template <typename R>
  using Value = typename yaclib::detail::InstantiationType<Destructive, R>::Value;

  template <typename V, typename... Args>
  static Destructive<V> MakeResult(Args&&... args) {
    static_assert(sizeof...(Args) > 0);
    using Head = std::decay_t<yaclib::head_t<Args&&...>>;
    if constexpr (sizeof...(Args) == 1 && std::is_same_v<Head, yaclib::Unit>) {
      return Destructive<V>{std::in_place};
    } else if constexpr (sizeof...(Args) == 1 && std::is_same_v<Head, std::exception_ptr>) {
      return Destructive<V>{DestructiveError{std::make_error_code(std::errc::io_error)}};
    } else if constexpr (std::is_same_v<Head, std::in_place_t> ||
                         (sizeof...(Args) == 1 &&
                          (std::is_same_v<Head, yaclib::StopTag> || std::is_same_v<Head, DestructiveError> ||
                           std::is_same_v<Head, Destructive<V>>))) {
      return Destructive<V>{std::forward<Args>(args)...};
    } else {
      return Destructive<V>{std::in_place, std::forward<Args>(args)...};
    }
  }

  template <typename V>
  static bool Ok(const Destructive<V>& r) noexcept {
    return static_cast<bool>(r);
  }

  template <typename R>
  static decltype(auto) GetValue(R&& r) noexcept {
    return std::forward<R>(r).Value();
  }

  template <typename R>
  static decltype(auto) GetError(R&& r) noexcept {
    return std::forward<R>(r).Error();
  }
};

// All<FirstFail>::Consume must copy (not move) the error from a live shared input core:
// retained SharedFuture copies must still observe an intact error after the combinator consumed it

TEST(WhenAll, FirstFailCopiesSharedError) {
  auto [sf, sp] = yaclib::MakeSharedContract<int, DestructiveTrait>();
  auto sf2 = sf;
  auto [f2, p2] = yaclib::MakeContract<int, DestructiveTrait>();

  auto all = yaclib::WhenAll<yaclib::FailPolicy::FirstFail>(std::move(sf), std::move(f2));

  std::move(sp).Set(DestructiveError{std::make_error_code(std::errc::invalid_argument)});
  std::move(p2).Set(1);

  auto combined = std::move(all).Get();
  EXPECT_FALSE(combined);
  ASSERT_TRUE(combined.Error().code != nullptr);
  EXPECT_EQ(*combined.Error().code, std::make_error_code(std::errc::invalid_argument));

  const auto& retained = sf2.Get();
  EXPECT_FALSE(retained);
  ASSERT_TRUE(retained.Error().code != nullptr);
  EXPECT_EQ(*retained.Error().code, std::make_error_code(std::errc::invalid_argument));
}

TEST(WhenAll, FirstFailCopiesSharedErrorDynamic) {
  static constexpr std::size_t kCount = 3;

  std::vector<yaclib::SharedFuture<int, DestructiveTrait>> futures;
  std::vector<yaclib::SharedFuture<int, DestructiveTrait>> retained;
  std::vector<yaclib::SharedPromise<int, DestructiveTrait>> promises;
  for (std::size_t i = 0; i != kCount; ++i) {
    auto [f, p] = yaclib::MakeSharedContract<int, DestructiveTrait>();
    retained.push_back(f);
    futures.push_back(std::move(f));
    promises.push_back(std::move(p));
  }

  auto all = yaclib::WhenAll<yaclib::FailPolicy::FirstFail>(futures.begin(), futures.end());

  std::move(promises[1]).Set(DestructiveError{std::make_error_code(std::errc::invalid_argument)});
  std::move(promises[0]).Set(0);
  std::move(promises[2]).Set(2);

  auto combined = std::move(all).Get();
  EXPECT_FALSE(combined);
  ASSERT_TRUE(combined.Error().code != nullptr);
  EXPECT_EQ(*combined.Error().code, std::make_error_code(std::errc::invalid_argument));

  const auto& failed = retained[1].Get();
  EXPECT_FALSE(failed);
  ASSERT_TRUE(failed.Error().code != nullptr);
  EXPECT_EQ(*failed.Error().code, std::make_error_code(std::errc::invalid_argument));

  const auto& ok0 = retained[0].Get();
  ASSERT_TRUE(ok0);
  EXPECT_EQ(ok0.Value(), 0);
  const auto& ok2 = retained[2].Get();
  ASSERT_TRUE(ok2);
  EXPECT_EQ(ok2.Value(), 2);
}

TEST(WhenAll, FailWithError) {
  auto f1 = yaclib::MakeFuture<void>(yaclib::StopTag{});
  auto f2 = yaclib::MakeFuture<void>(yaclib::Unit{});
  const auto all1 = yaclib::WhenAll(std::move(f1), std::move(f2)).Get();
  EXPECT_FALSE(all1);
  EXPECT_TRUE(yaclib::IsStop(all1.Error()));

  auto f3 = yaclib::MakeFuture<int>(yaclib::StopTag{});
  auto f4 = yaclib::MakeFuture<int>(3);
  const auto all2 = yaclib::WhenAll(std::move(f3), std::move(f4)).Get();
  EXPECT_FALSE(all2);
  EXPECT_TRUE(yaclib::IsStop(all2.Error()));
}

TEST(WhenAll, NoneRetireMovesFinishedSharedCores) {
  struct Counting {
    explicit Counting(int* c) : copies{c} {
    }
    Counting(const Counting& other) : copies{other.copies} {
      ++*copies;
    }
    Counting(Counting&& other) noexcept = default;
    Counting& operator=(const Counting& other) {
      copies = other.copies;
      ++*copies;
      return *this;
    }
    Counting& operator=(Counting&&) noexcept = default;

    int* copies;
  };
  int first = 0;
  int last = 0;
  auto [sf1, sp1] = yaclib::MakeSharedContract<Counting>();
  auto [sf2, sp2] = yaclib::MakeSharedContract<Counting>();
  auto all = yaclib::WhenAll<yaclib::FailPolicy::None>(std::move(sf1), std::move(sf2));
  std::move(sp1).Set(Counting{&first});
  std::move(sp2).Set(Counting{&last});
  auto result = std::move(all).Get();
  ASSERT_TRUE(result);
  // Retire moves from a core whose delivery already finished
  EXPECT_EQ(first, 0);
  // The input that triggers the combinator destruction is retired inside its own
  // delivery, where the core still holds the last callback refs, so it's copied
  EXPECT_EQ(last, 1);
}

TEST(WhenAll, NoneRetiresLiveSharedCore) {
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  auto retained = sf;
  auto all = yaclib::WhenAll<yaclib::FailPolicy::None>(std::move(sf));
  std::move(sp).Set(5);
  auto result = std::move(all).Get();
  ASSERT_TRUE(result);
  const auto& items = std::as_const(result).Value();
  ASSERT_EQ(items.size(), 1);
  EXPECT_EQ(std::as_const(items[0]).Value(), 5);
  // The combinator retired the shared core while this copy was still alive, so it copied the value
  EXPECT_EQ(retained.Get().Value(), 5);
}

}  // namespace
}  // namespace test
