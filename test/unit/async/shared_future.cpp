#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/async/share.hpp>
#include <yaclib/async/shared_contract.hpp>
#include <yaclib/async/shared_future.hpp>
#include <yaclib/async/shared_promise.hpp>
#include <yaclib/async/split.hpp>
#include <yaclib/exe/manual.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

#include <string_view>
#include <thread>

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

TEST(SharedFuture, SetGet) {
  SetGet<>(yaclib::Unit{});
  SetGet<int>(kSetInt);
  SetGet<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void SetGetFuture(Expected expected) {
  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
  std::move(sp).Set(expected);
  auto f = Share(sf);
  EXPECT_EQ(std::move(f).Get().Value(), expected);
}

TEST(SharedFuture, SetGetFuture) {
  SetGetFuture<>(yaclib::Unit{});
  SetGetFuture<int>(kSetInt);
  SetGetFuture<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void GetFutureSet(Expected expected) {
  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
  auto f = Share(sf);
  std::move(sp).Set(expected);
  EXPECT_EQ(std::move(f).Get().Value(), expected);
}

TEST(SharedFuture, GetFutureSet) {
  GetFutureSet<>(yaclib::Unit{});
  GetFutureSet<int>(kSetInt);
  GetFutureSet<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void GetFutureOn(Expected expected) {
  yaclib::FairThreadPool tp;

  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
  auto f = Share(sf, tp);
  std::move(f).Detach([=](Expected value) {
    EXPECT_EQ(value, expected);
  });

  std::move(sp).Set(expected);

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedFuture, GetFutureOn) {
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

  auto f1 = Share(sf);
  EXPECT_EQ(std::move(f1).Get().Value(), expected);

  {
    auto [sf_internal, sp_internal] = yaclib::MakeSharedContract<V, E>();
    sf = sf_internal;
  }
  auto f2 = Share(sf);
  EXPECT_EQ(std::move(f2).Get().Error(), yaclib::StopTag{});
}

TEST(SharedFuture, PromiseDtor) {
  PromiseDtor<>(yaclib::Unit{});
  PromiseDtor<int>(kSetInt);
  PromiseDtor<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void PromiseAttach(Expected expected) {
  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
  auto [f, p] = yaclib::MakeContract<V, E>();

  Connect(sf, std::move(p));
  std::move(sp).Set(expected);

  EXPECT_EQ(std::move(f).Get().Value(), expected);
}

TEST(SharedFuture, PromiseAttach) {
  PromiseAttach<>(yaclib::Unit{});
  PromiseAttach<int>(kSetInt);
  PromiseAttach<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void Concurrent(Expected expected) {
  yaclib::FairThreadPool tp;

  auto [sf, sp] = yaclib::MakeSharedContract<V, E>();

  for (std::size_t i = 0; i < 10; ++i) {
    yaclib::Submit(tp, [&sf = sf, &tp, expected]() {
      Share(sf).Detach(tp, [expected](Expected value) {
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

TEST(SharedFuture, Concurrent) {
  Concurrent<>(yaclib::Unit{});
  Concurrent<int>(kSetInt);
  Concurrent<std::string>(kSetString);
}

template <typename V = void, typename E = yaclib::StopError, typename Expected>
void FuturesOutliveShared(Expected expected) {
  yaclib::Future<V, E> f;

  {
    auto [sf, sp] = yaclib::MakeSharedContract<V, E>();
    f = Share(sf);
  }

  EXPECT_EQ(std::move(f).Get().Error(), yaclib::StopTag{});

  yaclib::SharedPromise<V, E> sp;
  {
    yaclib::SharedFuture<V, E> sf;
    std::tie(sf, sp) = yaclib::MakeSharedContract<V, E>();
    f = Share(sf);
  }

  std::move(sp).Set(expected);
  EXPECT_EQ(std::move(f).Get().Value(), expected);
}

TEST(SharedFuture, FuturesOutliveShared) {
  FuturesOutliveShared<>(yaclib::Unit{});
  FuturesOutliveShared<int>(kSetInt);
  FuturesOutliveShared<std::string>(kSetString);
}

TEST(SharedFuture, CopySharedFuture) {
  auto [sf1, sp] = yaclib::MakeSharedContract<int>();

  auto sf2 = sf1;

  ASSERT_TRUE(sf1.Valid());
  ASSERT_TRUE(sf2.Valid());

  auto f1 = Share(sf1);
  auto f2 = Share(sf2);

  std::move(sp).Set(kSetInt);

  ASSERT_EQ(std::move(f1).Get().Value(), kSetInt);
  ASSERT_EQ(std::move(f2).Get().Value(), kSetInt);
}

TEST(SharedFuture, MoveSharedFuture) {
  auto [sf1, sp] = yaclib::MakeSharedContract<int>();
  ASSERT_EQ(sf1.Valid(), true);

  auto sf2 = std::move(sf1);
  ASSERT_EQ(sf1.Valid(), false);
  ASSERT_EQ(sf2.Valid(), true);

  std::move(sp).Set(kSetInt);
  ASSERT_EQ(sf2.Get().Value(), kSetInt);
}

TEST(SharedFuture, MoveSharedPromise) {
  auto [sf, sp1] = yaclib::MakeSharedContract<int>();
  ASSERT_EQ(sp1.Valid(), true);

  auto sp2 = std::move(sp1);
  ASSERT_EQ(sp1.Valid(), false);
  ASSERT_EQ(sp2.Valid(), true);

  std::move(sp2).Set(kSetInt);
  ASSERT_EQ(sf.Get().Value(), kSetInt);
}

TEST(SharedFuture, SharedFutureGet) {
  yaclib::FairThreadPool tp;
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  yaclib::Submit(tp, [sp = std::move(sp)]() mutable {
    std::move(sp).Set(kSetInt);
  });

  ASSERT_EQ(sf.Get().Value(), kSetInt);
  ASSERT_EQ(Share(sf).Get().Value(), kSetInt);

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedFuture, SharedFutureConcurrentGet) {
  yaclib::FairThreadPool tp;
  auto [sf, sp] = yaclib::MakeSharedContract<int>();
  yaclib::Submit(tp, [sp = std::move(sp)]() mutable {
    std::move(sp).Set(kSetInt);
  });

  yaclib::FairThreadPool tp2;
  for (std::size_t i = 0; i < 10; ++i) {
    yaclib::Submit(tp2, [&sf = sf]() mutable {
      ASSERT_EQ(sf.Get().Value(), kSetInt);
    });
  }

  tp.SoftStop();
  tp.Wait();

  tp2.SoftStop();
  tp2.Wait();
}

TEST(SharedFuture, FutureToShared) {
  auto [f, p] = yaclib::MakeContract<int>();
  auto sf = Split(std::move(f));
  auto f1 = Share(sf);
  auto f2 = Share(sf);
  std::move(p).Set(kSetInt);
  ASSERT_EQ(std::move(f1).Get().Value(), kSetInt);
  ASSERT_EQ(std::move(f2).Get().Value(), kSetInt);
}

TEST(SharedFuture, FutureToSharedConcurrent) {
  yaclib::FairThreadPool tp(3);

  auto [f, p] = yaclib::MakeContract<int>();
  auto sf = Split(std::move(f));

  yaclib::Submit(tp, [&sf = sf]() mutable {
    auto f = Share(sf);
    ASSERT_EQ(std::move(f).Get().Value(), kSetInt);
  });

  yaclib::Submit(tp, [&sf = sf]() mutable {
    ASSERT_EQ(sf.Get().Value(), kSetInt);
  });

  yaclib::Submit(tp, [p = std::move(p)]() mutable {
    std::move(p).Set(kSetInt);
  });

  tp.SoftStop();
  tp.Wait();
}

static int copy_ctor{0};
static int move_ctor{0};

struct Counter {
  static void Init() {
    copy_ctor = 0;
    move_ctor = 0;
  }

  Counter() noexcept = default;
  Counter(const Counter&) noexcept {
    ++copy_ctor;
  }
  Counter(Counter&&) noexcept {
    ++move_ctor;
  }
  [[maybe_unused]] Counter& operator=(Counter&&) noexcept = default;
  Counter& operator=(const Counter&) noexcept = default;
  ~Counter() = default;
};

TEST(SharedFuture, MoveValueFromShared) {
  auto [sf, sp] = yaclib::MakeSharedContract<Counter>();

  auto f1 = Share(sf);
  auto f2 = Share(sf);
  auto f3 = Share(sf);

  sf = {};

  Counter::Init();
  std::move(sp).Set();

  ASSERT_EQ(copy_ctor, 2);
  ASSERT_EQ(move_ctor, 1);
}

TEST(SharedFuture, MakeSharedPromise) {
  auto sp = yaclib::MakeSharedPromise<Counter>();

  auto f1 = Share(sp);
  auto f2 = Share(sp);
  auto f3 = Share(sp);

  Counter::Init();
  std::move(sp).Set();

  ASSERT_EQ(copy_ctor, 2);
  ASSERT_EQ(move_ctor, 1);
}

TEST(SharedFuture, SharedPromiseShareOn) {
  yaclib::ManualExecutor e;
  auto sp = yaclib::MakeSharedPromise<int>();

  auto f1 = Share(sp);
  auto f2 = Share(sp, e);

  std::move(sp).Set(kSetInt);
  std::ignore = e.Drain();

  ASSERT_EQ(std::move(f1).Get().Value(), kSetInt);
  ASSERT_EQ(std::move(f2).Get().Value(), kSetInt);
}

TEST(SharedFuture, SharedPromiseSplit) {
  auto sp = yaclib::MakeSharedPromise<int>();

  auto sf = Split(sp);
  auto f = Share(sf);

  std::move(sp).Set(kSetInt);

  ASSERT_EQ(std::move(f).Get().Value(), kSetInt);
}

TEST(SharedFuture, ThenSet) {
  yaclib::FairThreadPool tp;
  auto [sf, sp] = yaclib::MakeSharedContract<int>();

  sf.Then(tp,
          [](int x) {
            return x + 1;
          })
    .Detach([](int x) {
      ASSERT_EQ(x, kSetInt + 1);
    });
  std::move(sp).Set(kSetInt);

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedFuture, SetThen) {
  yaclib::FairThreadPool tp;
  auto [sf, sp] = yaclib::MakeSharedContract<int>();

  std::move(sp).Set(kSetInt);
  sf.Then(tp,
          [](int x) {
            return x + 1;
          })
    .Detach([](int x) {
      ASSERT_EQ(x, kSetInt + 1);
    });

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedFuture, ThenSetThen) {
  yaclib::FairThreadPool tp;
  auto [sf, sp] = yaclib::MakeSharedContract<int>();

  sf.Then(tp,
          [](int x) {
            return x + 1;
          })
    .Detach([](int x) {
      ASSERT_EQ(x, kSetInt + 1);
    });
  std::move(sp).Set(kSetInt);
  sf.Then(tp,
          [](int x) {
            return x + 1;
          })
    .Detach([](int x) {
      ASSERT_EQ(x, kSetInt + 1);
    });

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedFuture, ThenOutlivesContract) {
  yaclib::ManualExecutor e;
  {
    auto [sf, sp] = yaclib::MakeSharedContract<int>();
    sf.Then(e,
            [](int x) {
              return x + 1;
            })
      .Detach([](int x) {
        ASSERT_EQ(x, kSetInt + 1);
      });

    std::move(sp).Set(kSetInt);
  }
  std::ignore = e.Drain();
}

TEST(SharedFuture, ThenInline) {
  auto [sf, sp] = yaclib::MakeSharedContract<int>();

  sf.ThenInline([](int x) {
      return x + 1;
    })
    .DetachInline([](int x) {
      ASSERT_EQ(x, kSetInt + 1);
    });
  std::move(sp).Set(kSetInt);
  sf.ThenInline([](int x) {
      return x + 1;
    })
    .DetachInline([](int x) {
      ASSERT_EQ(x, kSetInt + 1);
    });
}

TEST(SharedFuture, RunShared) {
  yaclib::FairThreadPool tp{4};
  auto lambda = [] {
    return std::this_thread::get_id();
  };

  auto sf1 = yaclib::RunShared(lambda);
  EXPECT_EQ(sf1.Get().Value(), std::this_thread::get_id());

  auto sf2 = yaclib::RunShared(tp, lambda);
  EXPECT_NE(sf2.Get().Value(), std::this_thread::get_id());

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedFuture, On) {
  yaclib::FairThreadPool tp{4};

  auto f = yaclib::RunShared(tp,
                             [] {
                               return kSetInt;
                             })
             .On(nullptr)
             .ThenInline([](int a) {
               return a + kSetInt;
             });

  ASSERT_EQ(std::move(f).Get().Value(), kSetInt + kSetInt);

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedFuture, Unwrapping) {
  yaclib::ManualExecutor e1;
  yaclib::ManualExecutor e2;
  auto [f, p] = yaclib::MakeContract<int>();

  auto res = std::move(f).Then(e1, [&](int x) {
    auto [sf, sp] = yaclib::MakeSharedContract<int>();

    yaclib::Run(e2, [x, sp = std::move(sp)]() mutable {
      std::move(sp).Set(x + 1);
    }).Detach();

    return sf;
  });

  std::move(p).Set(kSetInt);

  std::ignore = e1.Drain();
  std::ignore = e2.Drain();

  ASSERT_EQ(std::move(res).Get().Value(), kSetInt + 1);
}

TEST(SharedFuture, MultipleWait) {
  yaclib::FairThreadPool tp{4};
  auto lambda = [] {
    return kSetInt;
  };

  auto sf = yaclib::RunShared(tp, lambda);

  Wait(sf);
  ASSERT_EQ(sf.Get().Value(), kSetInt);

  auto sf1 = yaclib::RunShared(tp, lambda);
  auto sf2 = yaclib::RunShared(tp, lambda);
  auto f1 = yaclib::Run(tp, lambda);

  Wait(sf1, sf1, f1, sf2);

  ASSERT_EQ(sf1.Get().Value(), kSetInt);
  ASSERT_EQ(sf2.Get().Value(), kSetInt);
  ASSERT_EQ(std::move(f1).Get().Value(), kSetInt);

  tp.SoftStop();
  tp.Wait();
}

TEST(SharedFuture, MultipleWaitDynamic) {
  yaclib::FairThreadPool tp{4};
  auto lambda = [] {
    return kSetInt;
  };

  std::vector<yaclib::SharedFutureOn<int>> futures;

  futures.push_back(yaclib::RunShared(tp, lambda));

  Wait(futures.begin(), futures.end());

  ASSERT_EQ(futures[0].Get().Value(), kSetInt);

  futures.clear();

  for (std::size_t i = 0; i < 10; ++i) {
    futures.push_back(yaclib::RunShared(tp, lambda));
  }

  Wait(futures.begin(), futures.end());

  for (std::size_t i = 0; i < 10; ++i) {
    ASSERT_EQ(futures[i].Get().Value(), kSetInt);
  }

  tp.SoftStop();
  tp.Wait();
}

}  // namespace
}  // namespace test
