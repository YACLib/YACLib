#include <yaclib/async/async.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <iostream>
#include <string>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;
using namespace std::chrono_literals;

GTEST_TEST(simple, just_works) {
  auto [f, p] = async::MakeContract<int>();

  ASSERT_FALSE(f.IsReady());
  ASSERT_TRUE(f.IsValid());

  std::move(p).Set(42);

  ASSERT_TRUE(f.IsReady());
  ASSERT_EQ(std::move(f).Get(), 42);
  // ASSERT_FALSE(f.IsValid());
}

GTEST_TEST(simple, make_contract) {
  auto [f, p] = async::MakeContract<int>();

  ASSERT_FALSE(f.IsReady());

  std::move(p).Set(42);

  ASSERT_TRUE(f.IsReady());
  ASSERT_EQ(std::move(f).Get(), 42);
}

GTEST_TEST(simple, blocking_get_result) {
  static const std::string kMessage = "Hello, world!";

  auto tp = executor::MakeThreadPool(1);

  auto [f, p] = async::MakeContract<std::string>();
  tp->Execute([p = std::move(p)]() mutable {
    std::this_thread::sleep_for(1s);
    std::move(p).Set(kMessage);
  });
  {
    // test_helpers::CPUTimeBudgetGuard cpu_time_budget(100ms);
    auto result = std::move(f).Get();

    // ASSERT_TRUE(result.IsOk());
    ASSERT_EQ(result, kMessage);
  }
  tp->HardStop();
}

#if 0
SIMPLE_TEST(Exception) {
  auto [f, p] = MakeContract<std::string>();

  try {
    throw std::runtime_error("test");
  } catch (...) {
    std::move(p).SetError(std::current_exception());
  }

  ASSERT_THROW(std::move(f).GetValue(), std::runtime_error);
}
#endif

GTEST_TEST(simple, completed) {
  // auto f = Future<int>::Completed(7);
  auto [f, p] = async::MakeContract<int>();
  std::move(p).Set(7);
  ASSERT_TRUE(f.IsValid());
  auto result = std::move(f).Get();
  // ASSERT_TRUE(result.IsOk());
  ASSERT_EQ(/* * */ result, 7);
}
#if 0
std::error_code MakeTimedOutErrorCode() {
  return std::make_error_code(std::errc::timed_out);
}

SIMPLE_TEST(Failed) {
  auto f = Future<int>::Failed(MakeTimedOutErrorCode());
  ASSERT_TRUE(f.IsValid());
  auto result = std::move(f).GetResult();
  ASSERT_FALSE(result.IsOk());
}

SIMPLE_TEST(Invalid) {
  auto f = Future<int>::Invalid();
  ASSERT_FALSE(f.IsValid());
}
#endif

GTEST_TEST(simple, async_via) {
  auto tp = executor::MakeThreadPool(3);

  {
    auto good = []() -> std::string {
      // ExpectThread("tp");
      return "Hello!";
    };

    auto f = async::AsyncVia(tp, good);
    ASSERT_EQ(std::move(f).Get(), "Hello!");
  }

  //  {
  //    auto bad = []() -> int {
  //      ExpectThread("tp");
  //      throw std::logic_error("test");
  //    };
  //
  //    auto result = AsyncVia(tp, bad).GetResult();
  //    ASSERT_TRUE(result.HasError());
  //    ASSERT_THROW(result.ThrowIfError(), std::logic_error);
  //  }

  tp->SoftStop();
}

#if 0
// Subscribe

SIMPLE_TEST(Subscribe1) {
  auto [f, p] = MakeContract<int>();

  std::move(p).SetValue(17);

  bool called = false;

  std::move(f).Subscribe([&called](Result<int> result) {
    ASSERT_EQ(result.ValueOrThrow(), 17);
    called = true;
  });

  ASSERT_FALSE(f.IsValid());
  ASSERT_TRUE(called);
}

SIMPLE_TEST(Subscribe2) {
  auto [f, p] = MakeContract<int>();

  auto result = wheels::make_result::Throw<std::runtime_error>("test");
  std::move(p).Set(std::move(result));

  bool called = false;

  std::move(f).Subscribe([&called](Result<int> result) {
    ASSERT_TRUE(result.HasError());
    called = true;
  });

  ASSERT_TRUE(called);
}

SIMPLE_TEST(Subscribe3) {
  auto tp = MakeStaticThreadPool(1, "tp");

  auto [f, p] = MakeContract<std::string>();

  std::atomic<bool> called{false};

  std::move(f).Subscribe([&](Result<std::string> result) {
    ExpectThread("tp");
    ASSERT_EQ(result.ValueOrThrow(), "Hello!");
    called.store(true);
  });

  ASSERT_FALSE(f.IsValid());
  ASSERT_FALSE(called.load());

  tp->Execute([p = std::move(p)]() mutable {
    std::move(p).SetValue("Hello!");
  });

  tp->Join();

  ASSERT_TRUE(called.load());
}

SIMPLE_TEST(SubscribeVia1) {
  test_helpers::CPUTimeBudgetGuard cpu_time_budget(100ms);

  auto tp = MakeStaticThreadPool(1, "callbacks");

  auto [f, p] = MakeContract<int>();

  std::move(p).SetValue(17);

  std::atomic<bool> called = false;

  auto callback = [&called](Result<int> result) mutable {
    ExpectThread("callbacks");
    ASSERT_EQ(result.ValueOrThrow(), 17);
    called.store(true);
  };

  // Schedule to thread pool immediately
  std::move(f).Via(tp).Subscribe(callback);

  tp->Join();

  ASSERT_TRUE(called);
}

SIMPLE_TEST(SubscribeVia2) {
  test_helpers::CPUTimeBudgetGuard cpu_time_budget(100ms);

  auto tp1 = MakeStaticThreadPool(1, "tp1");
  auto tp2 = MakeStaticThreadPool(1, "tp2");

  auto [f, p] = MakeContract<int>();

  std::atomic<bool> called = false;

  auto callback = [&called](Result<int> result) mutable {
    ExpectThread("tp2");
    ASSERT_EQ(result.ValueOrThrow(), 42);
    called.store(true);
  };

  std::move(f).Via(tp2).Subscribe(callback);

  tp1->Execute([p = std::move(p)]() mutable {
    ExpectThread("tp1");
    std::this_thread::sleep_for(1s);
    std::move(p).SetValue(42);
  });

  tp1->Join();
  tp2->Join();

  ASSERT_TRUE(called);
}
// ???
SIMPLE_TEST(ViaDoesNotBlockThreadPool) {
  auto tp = MakeStaticThreadPool(1, "single");

  auto [f, p] = MakeContract<int>();

  test_helpers::OneShotEvent done;

  auto set_done = [&done](Result<int> result) {
    ExpectThread("single");
    ASSERT_EQ(result.ValueOrThrow(), 42);
    done.Set();
  };

  std::move(f).Via(tp).Subscribe(set_done);

  // Thread pool is idle
  AsyncVia(tp, []() -> Unit {
    return {};
  }).GetValue();

  std::move(p).SetValue(42);
  done.Await();

  tp->Join();
}
#endif
GTEST_TEST(simple, then) {
  auto [f, p] = async::MakeContract<int>();

  auto g = std::move(f).Then([](int v) {
    return v * 2 + 1;
  });
  std::move(p).Set(3);
  ASSERT_EQ(std::move(g).Get(), 7);
}

GTEST_TEST(simple, then_thread_pool) {
  auto tp = executor::MakeThreadPool(4);

  auto compute = [] {
    std::this_thread::sleep_for(1s);
    return 42;
  };

  auto process = [](int v) -> int {
    return v + 1;
  };

  async::Future<int> f1 = async::AsyncVia(tp, compute);

  async::Future<int> f2 = std::move(f1).Then(process);

  ASSERT_EQ(std::move(f2).Get(), 43);

  tp->HardStop();
}

class Unit {};

GTEST_TEST(simple, via_then) {
  auto tp = executor::MakeThreadPool(2);
  auto [f, p] = async::MakeContract<Unit>();

  auto g = std::move(f).ThenVia(tp, [](Unit) {
    // ExpectThread("test");
    return 42;
  });

  // Launch
  std::move(p).Set({});

  ASSERT_EQ(std::move(g).Get(), 42);

  tp->SoftStop();
}
GTEST_TEST(simple, then_executor) {
  auto tp = executor::MakeThreadPool(1);
  {
    std::cerr << " ";
    async::Promise<Unit> p;
    {
      auto f{p.MakeFuture()};
      {
        auto g = std::move(f).ThenVia(tp, [](Unit) {
          return 42;
        });
        tp->SoftStop();
        tp->Wait();
        std::cerr << " ";
      }
      std::cerr << " ";
    }
    std::cerr << " ";
  }
  std::cerr << " ";
  // ASSERT_EQ(g.GetExecutor(), tp);
}

GTEST_TEST(simple, pipeline) {
  auto tp = executor::MakeThreadPool(4);

  // Pipeline stages:

  auto first = []() -> int {
    return 42;
  };

  auto second = [](int value) {
    return value * 2;
  };

  auto third = [](int value) {
    return value + 1;
  };

  auto f = async::AsyncVia(tp, first).Then(second).Then(third);

  ASSERT_EQ(std::move(f).Get(), 42 * 2 + 1);

  tp->SoftStop();
  tp->Wait();
}
#if 0
SIMPLE_TEST(Errors1) {
  auto tp = MakeStaticThreadPool(4, "tp");

  // Pipeline stages:

  auto first = []() -> int {
    return 1;
  };

  auto second = [](int v) -> int {
    std::cout << v << " x 2" << std::endl;
    return v * 2;
  };

  auto third = [](int v) -> int {
    std::cout << v << " + 1" << std::endl;
    return v + 1;
  };

  auto error_handler = [](wheels::Error) -> int {
    return 42;
  };

  auto last = [](int v) -> int {
    ExpectThread("tp");
    return v + 11;
  };

  auto f = AsyncVia(tp, first)
               .Then(second)
               .Then(third)
               .Recover(error_handler)
               .Then(last);

  ASSERT_EQ(std::move(f).GetValue(), 14);

  tp->Join();
}

SIMPLE_TEST(Errors2) {
  auto tp = MakeStaticThreadPool(4, "tp");

  // Pipeline stages:

  auto first = []() -> int {
    throw std::runtime_error("first");
  };

  auto second = [](int v) -> int {
    std::cout << v << " x 2" << std::endl;
    return v * 2;
  };

  auto third = [](int v) -> int {
    std::cout << v << " + 1" << std::endl;
    return v + 1;
  };

  auto error_handler = [](wheels::Error) -> int {
    return 42;
  };

  auto last = [](int v) -> int {
    ExpectThread("tp");
    return v + 11;
  };

  auto pipeline = AsyncVia(tp, first)
                      .Then(second)
                      .Then(third)
                      .Recover(error_handler)
                      .Then(last);

  ASSERT_EQ(std::move(pipeline).GetValue(), 53);

  tp->Join();
}

SIMPLE_TEST(AsyncThenAfter) {
  test_helpers::CPUTimeBudgetGuard cpu_time_budget(100ms);

  auto [f, p] = MakeContract<Unit>();

  std::atomic<bool> done{false};

  auto pipeline = std::move(f)
                      .Then([](Unit) {
                        return After(1s);
                      })
                      .Then([](Unit) {
                        return After(500ms);
                      })
                      .Then([](Unit) {
                        return After(250ms);
                      })
                      .Then([&done](Unit) -> Unit {
                        done = true;
                        std::cout << "Finally!" << std::endl;
                        return {};
                      });

  // Launch
  std::move(p).SetValue({});

  std::this_thread::sleep_for(1250ms);
  ASSERT_FALSE(done);

  std::move(pipeline).GetValue();
  ASSERT_TRUE(done);
}
#endif

class Calculator {
 public:
  Calculator(executor::IThreadPoolPtr tp) : tp_(tp) {
  }

  async::Future<int> Increment(int value) {
    return async::AsyncVia(tp_, [value]() {
      std::this_thread::sleep_for(1s);
      return value + 1;
    });
  }

  async::Future<int> Double(int value) {
    return async::AsyncVia(tp_, [value]() {
      std::this_thread::sleep_for(1s);
      return value * 2;
    });
  }

 private:
  executor::IThreadPoolPtr tp_;
};

#if 0
GTEST_TEST(simple, async_calculator) {
  // test_helpers::CPUTimeBudgetGuard cpu_time_budget(100ms);

  auto tp = executor::MakeThreadPool(4);

  Calculator calculator(tp);

  auto pipeline = calculator
                      .Increment(1)
                      .Then([&](int value) {
                        return calculator.Double(value);
                      })
                      .Then([&](int value) {
                        return calculator.Increment(value);
                      });

  ASSERT_EQ(std::move(pipeline).GetValue(), 5);

  tp->SoftStop();
  tp->Wait();
}
#endif

GTEST_TEST(simple, pipeline2) {
  // test_helpers::CPUTimeBudgetGuard cpu_time_budget(100ms);

  auto tp1 = executor::MakeThreadPool(2);
  auto tp2 = executor::MakeThreadPool(3);

  auto [f, p] = async::MakeContract<std::string>();

  auto make_stage = [](int index, std::string label) {
    return [index, label](std::string path) {
      // ExpectThread(label);
      std::cout << "At stage " << index << std::endl;
      return path + "->" + std::to_string(index);
    };
  };

  auto almost_there = std::move(f)
                          .ThenVia(tp1, make_stage(1, "tp1"))
                          .Then(make_stage(2, "tp1"))
                          .ThenVia(tp2, make_stage(3, "tp2"));

  std::move(p).Set("start");
  std::this_thread::sleep_for(100ms);

  auto finally = std::move(almost_there)
                     .Then(make_stage(4, "tp2"))
                     .ThenVia(tp1, make_stage(5, "tp1"));

  ASSERT_EQ(std::move(finally).Get(), "start->1->2->3->4->5");

  tp1->HardStop();
  tp2->HardStop();
  tp1->Wait();
  tp2->Wait();
}

#if 0
// Combinators

// All

SIMPLE_TEST(All) {
  constexpr int kSize = 3;
  std::vector<Promise<int>> promises;
  std::vector<Future<int>> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = MakeContract<int>();
    futures.push_back(std::move(f));
    promises.push_back(std::move(p));
  }

  auto all = All(std::move(futures));

  ASSERT_FALSE(all.IsReady());

  std::move(promises[2]).SetValue(7);
  std::move(promises[0]).SetValue(3);

  // Still not completed
  ASSERT_FALSE(all.IsReady());

  std::move(promises[1]).SetValue(5);

  ASSERT_TRUE(all.IsReady());

  Result<std::vector<int>> ints = std::move(all).GetResult();
  ASSERT_TRUE(ints.IsOk());

  ASSERT_EQ(*ints, std::vector<int>({7, 3, 5}));
}

SIMPLE_TEST(AllFails) {
  constexpr int kSize = 3;
  std::vector<Promise<int>> promises;
  std::vector<Future<int>> futures;
  for (int i = 0; i < kSize; ++i) {
    auto [f, p] = MakeContract<int>();
    futures.push_back(std::move(f));
    promises.push_back(std::move(p));
  }

  auto all = All(std::move(futures));

  ASSERT_FALSE(all.IsReady());

  // First error
  std::move(promises[1]).SetError(test_helpers::MakeTestError());
  ASSERT_TRUE(all.IsReady());

  // Second error
  std::move(promises[0]).SetError(test_helpers::MakeTestError());

  auto all_result = std::move(all).GetResult();
  ASSERT_TRUE(all_result.HasError());
}

SIMPLE_TEST(AllEmptyInput) {
  auto all = All(std::vector<Future<int>>{});

  ASSERT_TRUE(all.IsReady());
  ASSERT_TRUE(std::move(all).GetResult().IsOk());
}

SIMPLE_TEST(AllMultiThreaded) {
  auto tp = MakeStaticThreadPool(4, "tp");

  auto async_value = [tp](int value) {
    return AsyncVia(tp, [value]() {
      ExpectThread("tp");
      std::this_thread::sleep_for(100ms);
      return value;
    });
  };

  static const size_t kValues = 16;

  std::vector<Future<int>> fs;
  for (size_t i = 0; i < kValues; ++i) {
    fs.push_back(async_value((int)i));
  }

  auto ints = All(std::move(fs)).GetValue();
  std::sort(ints.begin(), ints.end());

  ASSERT_EQ(ints.size(), kValues);
  for (int i = 0; i < (int)kValues; ++i) {
    ASSERT_EQ(ints[i], i);
  }

  tp->Join();
}

// FirstOf

SIMPLE_TEST(FirstOf) {
  test_helpers::CPUTimeBudgetGuard cpu_time_budget(100ms);
  test_helpers::WallTimeLimitGuard wall_time_limit(1200ms);

  auto first_of = FirstOf(AsyncValue<int>(1, 2s), AsyncValue<int>(2, 1s),
                          AsyncValue<int>(3, 3s));

  ASSERT_EQ(FirstOf(std::move(first_of)).GetValue(), 2);
}

SIMPLE_TEST(FirstOfWithErrors1) {
  std::vector<Future<int>> fs;

  auto first_of = FirstOf(AsyncError<int>(500ms), AsyncValue(13, 2s),
                          AsyncError<int>(1500ms), AsyncValue(42, 1s));

  ASSERT_EQ(std::move(first_of).GetValue(), 42);
}

SIMPLE_TEST(FirstOfWithErrors2) {
  // TODO
}

SIMPLE_TEST(FirstOfEmptyInput) {
  std::vector<Future<int>> inputs;
  auto first_of = FirstOf(std::move(inputs));
  ASSERT_FALSE(first_of.IsValid());
}

SIMPLE_TEST(FirstOfDontWaitAfterValue) {
  auto first_of = FirstOf(AsyncValue(1, 20s), AsyncValue(2, 500ms));
  ASSERT_EQ(std::move(first_of).GetValue(), 2);
}

SIMPLE_TEST(Via) {
  auto tp = MakeStaticThreadPool(2, "test");

  auto [f, p] = MakeContract<Unit>();

  auto answer = std::move(f).Via(tp).Then([](Unit) {
    ExpectThread("test");
    return 42;
  });

  std::move(p).SetValue({});
  ASSERT_EQ(std::move(answer).GetResult().ValueOrThrow(), 42);

  tp->Join();
}

SIMPLE_TEST(AsyncVia2) {
  auto tp = MakeStaticThreadPool(4, "test");

  auto f = AsyncVia(tp, []() {
    ExpectThread("test");
    return 42;
  });

  std::this_thread::sleep_for(1s);

  std::move(f).Subscribe([](Result<int>) {
    ExpectThread("test");
  });

  tp->Join();
}
#endif
}  // namespace
