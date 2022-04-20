#include <util/error_suite.hpp>
#include <util/inline_helper.hpp>

#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/executor/submit.hpp>
#include <yaclib/executor/thread_pool.hpp>
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
    EXPECT_THROW(std::move(r).Ok(), std::runtime_error);
  });
  EXPECT_TRUE(called);
}

TEST(DetachInline, ErrorSimple) {
  ErrorSimple<true>();
  ErrorSimple<false>();
}

template <bool Inline>
void AsyncSimple() {
  auto tp = yaclib::MakeThreadPool(1);
  auto [f, p] = yaclib::MakeContract<std::string>();
  bool called = false;
  InlineDetach<Inline>(std::move(f), [&](yaclib::Result<std::string> r) {
    EXPECT_EQ(&yaclib::CurrentThreadPool(), tp);
    EXPECT_EQ(std::move(r).Ok(), "Hello!");
    called = true;
  });
  EXPECT_FALSE(called);
  Submit(*tp, [p = std::move(p)]() mutable {
    std::move(p).Set("Hello!");
  });
  tp->Stop();
  tp->Wait();
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
    EXPECT_NO_THROW(std::move(f).Get().Ok());
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
    EXPECT_NO_THROW(std::move(f).Get().Ok());
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
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }

  {
    auto f = yaclib::MakeFuture<int>(5)
               .ThenInline([](auto&&) {
                 throw std::runtime_error{""};
               })
               .ThenInline([&](auto) {
                 flag();
                 return yaclib::MakeFuture<int>(5);
               });
    EXPECT_TRUE(std::exchange(the_flag, false));
    EXPECT_NO_THROW(std::move(f).Get().Ok());
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
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }

  {
    auto f = yaclib::MakeFuture<int>(5)
               .ThenInline([](auto&&) {
                 throw std::runtime_error{""};
               })
               .ThenInline([&](auto&&) mutable {
                 flag();
                 return yaclib::MakeFuture<int>(5);
               });
    EXPECT_TRUE(std::exchange(the_flag, false));
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }
}

TEST(ThenInline, Error) {
  ThenInlineError<true>();
  // ThenInlineError<false>();
}

template <bool Inline>
void ThenInlineCallback() {
  auto [f, p] = yaclib::MakeContract<void>();
  auto f1 = InlineThen<Inline>(std::move(f), [] {
  });
  std::move(std::move(p)).Set();
  EXPECT_TRUE(f1.Ready());
}

TEST(ThenInline, Callback) {
  ThenInlineCallback<true>();
  ThenInlineCallback<false>();
}
template <bool Inline>
void ThenInlineStopped() {
  auto [f, p] = yaclib::MakeContract<void>();
  bool ready = false;
  auto f1 = InlineThen<Inline>(std::move(f), [&] {
    ready = true;
  });
  std::move(f1).Stop();
  std::move(p).Set();
  EXPECT_FALSE(ready);
}

TEST(ThenInline, Stopped) {
  ThenInlineStopped<true>();
  ThenInlineStopped<false>();
}

template <bool Inline>
void ThenInlineAsync() {
  auto [f, p] = yaclib::MakeContract<int>();
  f = InlineThen<Inline>(std::move(f), [](int x) {
    return yaclib::MakeFuture(x + 2);
  });
  std::move(f).Stop();
  std::move(p).Set(3);
}

TEST(ThenInline, Async) {
  ThenInlineAsync<true>();
  ThenInlineAsync<false>();
}

template <bool Inline>
void FutureMakeFuture() {
  {
    auto f = yaclib::MakeFuture();
    bool ready = false;
    InlineThen<Inline>(std::move(f),
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
    InlineThen<Inline>(std::move(f),
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
    InlineThen<Inline>(std::move(f),
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
