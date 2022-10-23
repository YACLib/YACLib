#include <util/async_suite.hpp>
#include <util/error_suite.hpp>
#include <util/inline_helper.hpp>

#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/make.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/exe/manual.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <exception>
#include <iosfwd>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace test {
namespace {

template <bool Inline>
void ValueSimple() {
  auto [f, p] = yaclib::MakeContract<int>();
  std::move(p).Set(17);
  bool called = false;
  InlineDetach<Inline>(std::move(f), [&called](int r) {
    EXPECT_EQ(r, 17);
    called = true;
  });
  EXPECT_TRUE(called);
}

TEST(DetachInline, ValueSimple) {
  ValueSimple<true>();
  ValueSimple<false>();
}

template <bool Inline>
void ResultSimple() {
  auto [f, p] = yaclib::MakeContract<int>();
  std::move(p).Set(17);
  bool called = false;
  InlineDetach<Inline>(std::move(f), [&called](yaclib::Result<int> r) {
    EXPECT_EQ(std::move(r).Ok(), 17);
    called = true;
  });
  EXPECT_TRUE(called);
}

TEST(DetachInline, ResultSimple) {
  ResultSimple<true>();
  ResultSimple<false>();
}

template <bool Inline>
void ErrorSimple() {
  auto [f, p] = yaclib::MakeContract<int>();
  auto result = yaclib::Result<int>{std::make_exception_ptr(std::runtime_error{""})};
  std::move(p).Set(std::move(result));
  bool called = false;
  InlineDetach<Inline>(std::move(f), [&called](yaclib::Result<int> r) {
    called = true;
    EXPECT_THROW(std::ignore = std::move(r).Ok(), std::runtime_error);
  });
  EXPECT_TRUE(called);
}

TEST(DetachInline, ErrorSimple) {
#if defined(GTEST_OS_WINDOWS) && !defined(_WIN64)
  GTEST_SKIP();  // Random crash because of EXPECT THROW macro with RelWithDebInfo
#endif
  ErrorSimple<true>();
  ErrorSimple<false>();
}

template <bool Inline>
void AsyncSimple() {
  yaclib::FairThreadPool tp{1};
  auto [f, p] = yaclib::MakeContract<std::string>();
  bool called = false;
  InlineDetach<Inline>(std::move(f), [&](yaclib::Result<std::string> r) {
    // EXPECT_EQ(&yaclib::CurrentThreadPool(), tp);
    EXPECT_EQ(std::move(r).Ok(), "Hello!");
    called = true;
  });
  EXPECT_FALSE(called);
  Submit(tp, [p = std::move(p)]() mutable {
    std::move(p).Set("Hello!");
  });
  tp.Stop();
  tp.Wait();
  EXPECT_TRUE(called);
}

TEST(DetachInline, AsyncSimple) {
  AsyncSimple<true>();
  AsyncSimple<false>();
}

TEST(ThenInline, Simple) {
  auto [f, p] = yaclib::MakeContract<int>();
  auto g = std::move(f).ThenInline([](int v) {
    return v * 2 + 1;
  });
  std::move(p).Set(3);
  EXPECT_EQ(std::move(g).Get().Ok(), 7);
}

template <typename TestType, bool Inline>
void ErrorThenInline() {
  static constexpr bool kIsError = !std::is_same_v<TestType, std::exception_ptr>;
  using ErrorType = std::conditional_t<kIsError, TestType, yaclib::StopError>;

  auto f = yaclib::MakeFuture<int, ErrorType>(32);
  auto f_async = InlineThen<Inline>(std::move(f), [](TestType) {
    return yaclib::MakeFuture<int, ErrorType>(42);
  });
  f = InlineThen<Inline>(std::move(f_async), [](yaclib::Result<int, ErrorType>) {
    return 2;
  });
  EXPECT_EQ(std::move(f).Get().Ok(), 2);
  auto make = [&] {
    f = yaclib::MakeFuture<int, ErrorType>(32);
    f = InlineThen<Inline>(std::move(f), [](int) -> yaclib::Result<int, ErrorType> {
      if constexpr (kIsError) {
        return yaclib::StopTag{};
      } else {
        throw std::runtime_error{""};
      }
      return 0;
    });
  };
  make();
  auto f_async2 = InlineThen<Inline>(std::move(f), [](int) {
    return yaclib::MakeFuture<double, ErrorType>(1.0);
  });
  f = InlineThen<Inline>(std::move(f_async2), [](yaclib::Result<double, ErrorType>) {
    return 2;
  });
  EXPECT_EQ(std::move(f).Get().Ok(), 2);

  make();
  f_async = InlineThen<Inline>(std::move(f), [](TestType) {
    return yaclib::MakeFuture<int, ErrorType>(2);
  });
  EXPECT_EQ(std::move(f_async).Get().Ok(), 2);
}

TYPED_TEST(Error, AsyncThenInline) {
  using TestType = typename TestFixture::Type;
  ErrorThenInline<TestType, true>();
  ErrorThenInline<TestType, false>();
}

template <bool Inline>
void ThenInlineError() {
  bool the_flag = false;
  auto flag = [&] {
    the_flag = true;
  };

  // By reference
  {
    auto f = yaclib::MakeFuture<int>(1)
               .ThenInline([](auto&&) {
                 throw std::runtime_error{""};
               })
               .ThenInline([&](auto&&) {
                 flag();
               });
    EXPECT_TRUE(std::exchange(the_flag, false));
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }

  {
    auto f = yaclib::MakeFuture<int>(5)
               .ThenInline([](auto&&) {
                 throw std::runtime_error{""};
               })
               .ThenInline([&](auto&&) {
                 flag();
                 return yaclib::MakeFuture<int>(6);
               });
    EXPECT_TRUE(std::exchange(the_flag, false));
    EXPECT_EQ(std::move(f).Get().Ok(), 6);
  }

  // By value
  {
    auto f = yaclib::MakeFuture<int>(5)
               .ThenInline([](auto&&) {
                 throw std::runtime_error{""};
               })
               .ThenInline([&](auto) {
                 flag();
               });
    EXPECT_TRUE(std::exchange(the_flag, false));
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }

  {
    auto f = yaclib::MakeFuture<int>(5)
               .ThenInline([](auto&&) {
                 throw std::runtime_error{""};
               })
               .ThenInline([&](auto) {
                 flag();
                 return yaclib::MakeFuture<int>(6);
               });
    EXPECT_TRUE(std::exchange(the_flag, false));
    EXPECT_EQ(std::move(f).Get().Ok(), 6);
  }

  // Mutable lambda
  {
    auto f = yaclib::MakeFuture<int>(5)
               .ThenInline([](auto&&) {
                 throw std::runtime_error{""};
               })
               .ThenInline([&](auto&&) mutable {
                 flag();
               });
    EXPECT_TRUE(std::exchange(the_flag, false));
    EXPECT_EQ(std::move(f).Get().Ok(), yaclib::Unit{});
  }

  {
    auto f = yaclib::MakeFuture<int>(5)
               .ThenInline([](auto&&) {
                 throw std::runtime_error{""};
               })
               .ThenInline([&](auto&&) mutable {
                 flag();
                 return yaclib::MakeFuture<int>(6);
               });
    EXPECT_TRUE(std::exchange(the_flag, false));
    EXPECT_EQ(std::move(f).Get().Ok(), 6);
  }
}

TEST(ThenInline, Error) {
  ThenInlineError<true>();
  ThenInlineError<false>();
}

template <bool Inline>
void ThenInlineCallback() {
  auto [f, p] = yaclib::MakeContract<>();
  auto f1 = InlineThen<Inline>(std::move(f), [] {
  });
  std::move(std::move(p)).Set();
  EXPECT_TRUE(f1.Ready());
}

TEST(ThenInline, Callback) {
  ThenInlineCallback<true>();
  ThenInlineCallback<false>();
}

class StopSource final : public yaclib::IExecutor {
 public:
  explicit StopSource(yaclib::IExecutor& executor) noexcept : _executor{executor} {
  }

  [[nodiscard]] Type Tag() const noexcept final {
    return Type::Custom;
  }

  [[nodiscard]] bool Alive() const noexcept final {
    return !_stop.load(std::memory_order_relaxed);
  }

  void Submit(yaclib::Job& job) noexcept final {
    if (Alive()) {
      _executor.Submit(job);
    } else {
      job.Drop();
    }
  }

  void Stop() noexcept {
    _stop.store(true, std::memory_order_relaxed);
  }

 private:
  yaclib::IExecutor& _executor;
  yaclib_std::atomic_bool _stop{false};
};

TYPED_TEST(AsyncSuite, Stop) {
  yaclib::FairThreadPool tp{1};
  StopSource source{tp};
  source.Stop();
  bool ready = false;
  INVOKE(source, [&] {
    ready = true;
  }).Detach();
  EXPECT_FALSE(ready);
  tp.Stop();
  tp.Wait();
}

template <typename TestFixture, bool Inline>
void ThenInlineStopped() {
  auto manual = yaclib::MakeManual();
  StopSource source{*manual};
  std::size_t ready = 0;
  auto inc = [&] {
    ++ready;
  };
  auto f = INVOKE(source, inc);
  auto f1 = InlineThen<Inline>(std::move(f), inc);
  auto f2 = [&] {
    if constexpr (Inline) {
      return std::move(f1).Then(inc);
    } else {
      return std::move(f1).Then(source, inc);
    }
  }();
  InlineThen<Inline>(std::move(f2), inc).Detach();
  source.Stop();
  EXPECT_EQ(static_cast<yaclib::ManualExecutor&>(*manual).Drain(), 1);
  EXPECT_EQ(ready, 2);
}

TYPED_TEST(AsyncSuite, ThenInlineStopped) {
  ThenInlineStopped<TestFixture, true>();
  ThenInlineStopped<TestFixture, false>();
}

template <bool Inline>
void ThenInlineAsync() {
  auto [f, p] = yaclib::MakeContract<int>();
  f = InlineThen<Inline>(std::move(f), [](int x) {
    return yaclib::MakeFuture(x + 2);
  });
  std::move(f).Detach();
  std::move(p).Set(3);
}

TEST(ThenInline, Async) {
  ThenInlineAsync<true>();
  ThenInlineAsync<false>();
}

template <typename Func>
auto MakeFinally(Func&& func) {
  using F = std::decay_t<Func>;
  auto deleter = [](F* p) {
    std::forward<Func> (*p)();
    delete p;
  };
  return std::unique_ptr<F, decltype(deleter)>{new F{std::forward<Func>(func)}, deleter};
}

TEST(ThenInline, OrderFunctorDestroy) {
  int x = 0;
  yaclib::ManualExecutor e;
  auto f1 = [&]() noexcept {
    EXPECT_EQ(x, 0);
    x += 10;
  };
  auto f2 = [&]() noexcept {
    EXPECT_EQ(x, 10);
    x += 100;
    EXPECT_EQ(1U, e.Drain());
    EXPECT_EQ(x, 1110);
  };
  auto f3 = [&]() noexcept {
    EXPECT_EQ(x, 110);
    x += 1000;
  };
  auto f4 = [&]() noexcept {
    EXPECT_EQ(x, 1110);
    x += 10000;
  };
  auto f5 = [&]() noexcept {
    EXPECT_EQ(x, 11110);
    x += 100000;
  };
  auto f6 = [&]() noexcept {
    EXPECT_EQ(x, 111110);
    x += 1000000;
  };
  auto f = yaclib::MakeFuture()
             .ThenInline([&, d1 = MakeFinally(f1)] {
               EXPECT_EQ(x, 0);
             })
             .ThenInline([&, d2 = MakeFinally(f2)] {
               EXPECT_EQ(x, 10);
               return yaclib::Run(e, [&, d3 = MakeFinally(f3)] {
                 EXPECT_EQ(x, 110);
               });
             })
             .ThenInline([&, d5 = MakeFinally(f5)] {
               EXPECT_EQ(x, 1110);
               auto temp = yaclib::Run(yaclib::MakeInline(), [&, d4 = MakeFinally(f4)] {
                 EXPECT_EQ(x, 1110);
               });
               EXPECT_EQ(x, 11110);
               return temp;
             })
             .ThenInline([&, d6 = MakeFinally(f6)] {
               EXPECT_EQ(x, 111110);
             });
  EXPECT_TRUE(f.Ready());
  EXPECT_EQ(x, 1111110);
};

template <bool Inline>
void FutureMakeFuture() {
  {
    auto f = yaclib::MakeFuture();
    bool ready = false;
    std::ignore = InlineThen<Inline>(std::move(f),
                                     [&] {
                                       ready = true;
                                     })
                    .Get()
                    .Ok();
    EXPECT_TRUE(ready);
  }
  {
    auto f = yaclib::MakeFuture(1.0F);
    bool ready = false;
    std::ignore = InlineThen<Inline>(std::move(f),
                                     [&](float) {
                                       ready = true;
                                     })
                    .Get()
                    .Ok();
    EXPECT_TRUE(ready);
  }
  {
    auto f = yaclib::MakeFuture<double>(1.0F);
    bool ready = false;
    std::ignore = InlineThen<Inline>(std::move(f),
                                     [&](double) {
                                       ready = true;
                                     })
                    .Get()
                    .Ok();
    EXPECT_TRUE(ready);
  }
}

TEST(Future, MakeFuture) {
  FutureMakeFuture<true>();
  FutureMakeFuture<false>();
}

}  // namespace
}  // namespace test
