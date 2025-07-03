#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/shared_contract.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

#include <string_view>

#include <gtest/gtest.h>

namespace test {
namespace {

constexpr int kSetInt = 5;
constexpr std::string_view kSetString = "aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa";

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void SetGet(Expected expected) {
  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
  std::move(sp).Set(expected);
  EXPECT_EQ(sf.Get().Value(), expected);
}

TEST(SharedCore, SetGet) {
  SetGet<>(yaclib::Unit{});
  SetGet<int>(kSetInt);
  SetGet<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void SetGetFuture(Expected expected) {
  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
  std::move(sp).Set(expected);
  auto f = sf.GetFuture();
  EXPECT_EQ(std::move(f).Get().Value(), expected);
}

TEST(SharedCore, SetGetFuture) {
  SetGetFuture<>(yaclib::Unit{});
  SetGetFuture<int>(kSetInt);
  SetGetFuture<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void GetFutureSet(Expected expected) {
  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
  auto f = sf.GetFuture();
  std::move(sp).Set(expected);
  EXPECT_EQ(std::move(f).Get().Value(), expected);
}

TEST(SharedCore, GetFutureSet) {
  GetFutureSet<>(yaclib::Unit{});
  GetFutureSet<int>(kSetInt);
  GetFutureSet<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void GetFutureOn(Expected expected) {
  yaclib::FairThreadPool tp;

  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
  auto f = sf.GetFutureOn(tp);
  std::move(f).Detach([=](Expected value) {
    EXPECT_EQ(value, expected);
  });

  std::move(sp).Set(expected);

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedCore, GetFutureOn) {
  GetFutureOn<>(yaclib::Unit{});
  GetFutureOn<int>(kSetInt);
  GetFutureOn<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void PromiseDtor(Expected expected) {
  yaclib::SharedFuture<V, E> sf;

  {
    auto [sf_internal, sp_internal] = yaclib::MakeSharedContract<V, E>();
    std::move(sp_internal).Set(expected);
    sf = sf_internal;
  }

  auto f1 = sf.GetFuture();
  EXPECT_EQ(std::move(f1).Get().Value(), expected);

  {
    auto [sf_internal, sp_internal] = yaclib::MakeSharedContract<V, E>();
    sf = sf_internal;
  }
  auto f2 = sf.GetFuture();
  EXPECT_EQ(std::move(f2).Get().Error(), yaclib::StopTag{});
}

TEST(SharedCore, PromiseDtor) {
  PromiseDtor<>(yaclib::Unit{});
  PromiseDtor<int>(kSetInt);
  PromiseDtor<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void PromiseAttach(Expected expected) {
  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
  auto [f, p] = yaclib::MakeContract<V, E>();

  sf.Attach(std::move(p));
  std::move(sp).Set(expected);

  EXPECT_EQ(std::move(f).Get().Value(), expected);
}

TEST(SharedCore, PromiseAttach) {
  PromiseAttach<>(yaclib::Unit{});
  PromiseAttach<int>(kSetInt);
  PromiseAttach<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void Concurrent(Expected expected) {
  yaclib::FairThreadPool tp;

  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();

  for (size_t i = 0; i < 10; ++i) {
    yaclib::Submit(tp, [&sf = sf, &tp, expected]() {
      sf.GetFuture().Detach(tp, [expected](Expected value) {
        EXPECT_EQ(value, expected);
      });
    });
  }

  yaclib::Submit(tp, [sp = std::move(sp), expected]() mutable {
    std::move(sp).Set(expected);
  });

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedCore, Concurrent) {
  Concurrent<>(yaclib::Unit{});
  Concurrent<int>(kSetInt);
  Concurrent<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void FuturesOutliveShared(Expected expected) {
  yaclib::Future<V, E> f;

  {
    auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
    f = sf.GetFuture();
  }

  EXPECT_EQ(std::move(f).Get().Error(), yaclib::StopTag{});

  yaclib::SharedPromise<V, E> sp;
  {
    yaclib::SharedFuture<V, E> sf;
    std::tie(sf, sp) = yaclib::MakeSharedContract<V, E>();
    f = sf.GetFuture();
  }

  std::move(sp).Set(expected);
  EXPECT_EQ(std::move(f).Get().Value(), expected);
}

TEST(SharedCore, FuturesOutliveShared) {
  FuturesOutliveShared<>(yaclib::Unit{});
  FuturesOutliveShared<int>(kSetInt);
  FuturesOutliveShared<std::string>(kSetString);
}

TEST(SharedCore, CopySharedFuture) {
  auto [sf1, sp] = yaclib::MakeSharedContract<int>();

  auto sf2 = sf1;

  ASSERT_TRUE(sf1.Valid());
  ASSERT_TRUE(sf2.Valid());

  auto f1 = sf1.GetFuture();
  auto f2 = sf2.GetFuture();

  std::move(sp).Set(kSetInt);

  ASSERT_EQ(std::move(f1).Get().Value(), kSetInt);
  ASSERT_EQ(std::move(f2).Get().Value(), kSetInt);
}

TEST(SharedCore, MoveSharedFuture) {
  auto [sf1, sp] = yaclib::MakeSharedContract<int>();
  ASSERT_EQ(sf1.Valid(), true);

  auto sf2 = std::move(sf1);
  ASSERT_EQ(sf1.Valid(), false);
  ASSERT_EQ(sf2.Valid(), true);

  std::move(sp).Set(kSetInt);
  ASSERT_EQ(sf2.Get().Value(), kSetInt);
}

TEST(SharedCore, MoveSharedPromise) {
  auto [sf, sp1] = yaclib::MakeSharedContract<int>();
  ASSERT_EQ(sp1.Valid(), true);

  auto sp2 = std::move(sp1);
  ASSERT_EQ(sp1.Valid(), false);
  ASSERT_EQ(sp2.Valid(), true);

  std::move(sp2).Set(kSetInt);
  ASSERT_EQ(sf.Get().Value(), kSetInt);
}

TEST(SharedCore, SharedFutureGet) {
  yaclib::FairThreadPool tp;
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  yaclib::Submit(tp, [sp = std::move(sp)]() mutable {
    std::move(sp).Set(kSetInt);
  });

  ASSERT_EQ(sf.Get().Value(), kSetInt);
  ASSERT_EQ(sf.GetFuture().Get().Value(), kSetInt);

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedCore, SharedFutureConcurrentGet) {
  yaclib::FairThreadPool tp;
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  yaclib::Submit(tp, [sp = std::move(sp)]() mutable {
    std::move(sp).Set(kSetInt);
  });

  yaclib::FairThreadPool tp2;
  for (size_t i = 0; i < 10; ++i) {
    yaclib::Submit(tp2, [&sf = sf]() mutable {
      ASSERT_EQ(sf.Get().Value(), kSetInt);
    });
  }

  tp.SoftStop();
  tp.Wait();

  tp2.SoftStop();
  tp2.Wait();
}

}  // namespace
}  // namespace test
