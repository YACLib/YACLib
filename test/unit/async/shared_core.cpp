#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/shared_contract.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

#include <gtest/gtest.h>

namespace test {
namespace {

constexpr int SET_VALUE = 5;

TEST(SharedCore, Void) {
  auto [sf, sp] = yaclib::MakeSharedContract<>();
  std::move(sp).Set();
  EXPECT_EQ(sf.Get().Value(), yaclib::Unit{});
}

TEST(SharedCore, SetGet) {
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  std::move(sp).Set(SET_VALUE);
  EXPECT_EQ(sf.Get().Value(), SET_VALUE);
}

TEST(SharedCore, SetGetFuture) {
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  std::move(sp).Set(SET_VALUE);
  auto f = sf.GetFuture();
  EXPECT_EQ(std::move(f).Get().Value(), SET_VALUE);
}

TEST(SharedCore, GetFutureSet) {
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  auto f = sf.GetFuture();
  std::move(sp).Set(SET_VALUE);
  EXPECT_EQ(std::move(f).Get().Value(), SET_VALUE);
}

TEST(SharedCore, GetFutureOn) {
  yaclib::FairThreadPool tp;

  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  auto f = sf.GetFutureOn(tp);
  std::move(f).Detach([](int value) {
    EXPECT_EQ(value, SET_VALUE);
  });

  std::move(sp).Set(SET_VALUE);

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedCore, PromiseDtor) {
  yaclib::SharedFuture<int> sf;

  {
    auto [sf_internal, sp_internal] = yaclib::MakeSharedContract<int>();
    std::move(sp_internal).Set(SET_VALUE);
    sf = sf_internal;
  }

  auto f1 = sf.GetFuture();
  EXPECT_EQ(std::move(f1).Get().Value(), SET_VALUE);

  {
    auto [sf_internal, sp_internal] = yaclib::MakeSharedContract<int>();
    sf = sf_internal;
  }
  auto f2 = sf.GetFuture();
  EXPECT_EQ(std::move(f2).Get().Error(), yaclib::StopTag{});
}

TEST(SharedCore, PromiseAttach) {
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  auto [f, p] = yaclib::MakeContract<int>();

  sf.Attach(std::move(p));
  std::move(sp).Set(SET_VALUE);

  EXPECT_EQ(std::move(f).Get().Value(), SET_VALUE);
}

TEST(SharedCore, Concurrent) {
  yaclib::FairThreadPool tp;

  auto [sf, sp] = yaclib::MakeSharedContract<int>();

  for (size_t i = 0; i < 10; ++i) {
    yaclib::Submit(tp, [&sf = sf, &tp]() {
      sf.GetFuture().Detach(tp, [](int value) {
        EXPECT_EQ(value, SET_VALUE);
      });
    });
  }

  yaclib::Submit(tp, [sp = std::move(sp)]() mutable {
    std::move(sp).Set(SET_VALUE);
  });

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedCore, FuturesOutliveShared) {
  yaclib::Future<int> f;

  {
    auto [sf, sp] = yaclib::MakeSharedContract<int>();
    f = sf.GetFuture();
  }

  EXPECT_EQ(std::move(f).Get().Error(), yaclib::StopTag{});

  yaclib::SharedPromise<int> sp;
  {
    yaclib::SharedFuture<int> sf;
    std::tie(sf, sp) = yaclib::MakeSharedContract<int>();
    f = sf.GetFuture();
  }

  std::move(sp).Set(SET_VALUE);
  EXPECT_EQ(std::move(f).Get().Value(), SET_VALUE);
}

TEST(SharedCore, CopySharedFuture) {
  auto [sf1, sp] = yaclib::MakeSharedContract<int>();

  auto sf2 = sf1;

  ASSERT_TRUE(sf1.Valid());
  ASSERT_TRUE(sf2.Valid());

  auto f1 = sf1.GetFuture();
  auto f2 = sf2.GetFuture();

  std::move(sp).Set(SET_VALUE);

  ASSERT_EQ(std::move(f1).Get().Value(), SET_VALUE);
  ASSERT_EQ(std::move(f2).Get().Value(), SET_VALUE);
}

TEST(SharedCore, MoveSharedFuture) {
  auto [sf1, sp] = yaclib::MakeSharedContract<int>();
  ASSERT_EQ(sf1.Valid(), true);

  auto sf2 = std::move(sf1);
  ASSERT_EQ(sf1.Valid(), false);
  ASSERT_EQ(sf2.Valid(), true);

  std::move(sp).Set(SET_VALUE);
  ASSERT_EQ(sf2.Get().Value(), SET_VALUE);
}

TEST(SharedCore, MoveSharedPromise) {
  auto [sf, sp1] = yaclib::MakeSharedContract<int>();
  ASSERT_EQ(sp1.Valid(), true);

  auto sp2 = std::move(sp1);
  ASSERT_EQ(sp1.Valid(), false);
  ASSERT_EQ(sp2.Valid(), true);

  std::move(sp2).Set(SET_VALUE);
  ASSERT_EQ(sf.Get().Value(), SET_VALUE);
}

TEST(SharedCore, SharedFutureGet) {
  yaclib::FairThreadPool tp;
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  yaclib::Submit(tp, [sp = std::move(sp)]() mutable {
    std::move(sp).Set(SET_VALUE);
  });

  ASSERT_EQ(sf.Get().Value(), SET_VALUE);
  ASSERT_EQ(sf.GetFuture().Get().Value(), SET_VALUE);

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedCore, SharedFutureConcurrentGet) {
  yaclib::FairThreadPool tp;
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  yaclib::Submit(tp, [sp = std::move(sp)]() mutable {
    std::move(sp).Set(SET_VALUE);
  });

  yaclib::FairThreadPool tp2;
  for (size_t i = 0; i < 10; ++i) {
    yaclib::Submit(tp2, [&sf = sf]() mutable {
      ASSERT_EQ(sf.Get().Value(), SET_VALUE);
    });
  }

  tp.SoftStop();
  tp.Wait();

  tp2.SoftStop();
  tp2.Wait();
}

}  // namespace
}  // namespace test
