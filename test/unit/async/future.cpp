#include <util/intrusive_list.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <iostream>
#include <string>

#include <gtest/gtest.h>

namespace {

using namespace std::chrono_literals;

template <typename FutureType, typename ErrorType>
void ErrorsCheck(ErrorType expected) {
  static_assert(std::is_same_v<ErrorType, std::exception_ptr> || std::is_same_v<ErrorType, std::error_code>);
  auto [f, p] = yaclib::MakeContract<FutureType>();
  EXPECT_FALSE(f.Ready());
  std::move(p).Set(expected);
  EXPECT_TRUE(f.Ready());
  auto result = std::move(f).Get();
  if constexpr (std::is_same_v<ErrorType, std::exception_ptr>) {
    EXPECT_EQ(result.State(), yaclib::util::ResultState::Exception);
    EXPECT_EQ(std::move(result).Exception(), expected);
  } else {
    EXPECT_EQ(result.State(), yaclib::util::ResultState::Error);
    EXPECT_EQ(std::move(result).Error(), expected);
  }
}

// FutureType must be default constructible
template <typename FutureType>
void ValueCheck() {
  auto [f, p] = yaclib::MakeContract<FutureType>();
  EXPECT_FALSE(f.Ready());
  if constexpr (std::is_void_v<FutureType>) {
    std::move(p).Set();
  } else {
    std::move(p).Set(FutureType{});
  }
  EXPECT_TRUE(f.Ready());
  auto result = std::move(f).Get();
  EXPECT_EQ(result.State(), yaclib::util::ResultState::Value);
  if constexpr (!std::is_void_v<FutureType>) {
    EXPECT_EQ(std::move(result).Value(), FutureType{});
  } else {
    using ResultType = decltype(std::move(result).Value());
    using ExpectedType = decltype(yaclib::util::Result<FutureType>::Default().Value());
    static_assert(std::is_same_v<ResultType, ExpectedType>);
  }
}

void AsyncGetResult(int num_threads) {
  static const std::string kMessage = "Hello, world!";
  auto tp = yaclib::MakeThreadPool(num_threads);

  auto [f, p] = yaclib::MakeContract<std::string>();
  tp->Execute([p = std::move(p)]() mutable {
    std::move(p).Set(kMessage);
  });
  EXPECT_EQ(std::move(f).Get().Ok(), kMessage);
  tp->HardStop();
  tp->Wait();
}

TEST(JustWorks, Value) {
  ValueCheck<double>();
}

TEST(JustWorks, ErrorCode) {
  ErrorsCheck<double>(std::error_code{});
}

TEST(JustWorks, Exception) {
  ErrorsCheck<double>(std::make_exception_ptr(std::runtime_error{""}));
}

TEST(JustWorks, Run) {
  auto tp = yaclib::MakeThreadPool();
  bool called = false;
  auto f = yaclib::Run(tp, [&] {
    called = true;
  });
  Wait(f);
  EXPECT_TRUE(called);
  tp->Stop();
  tp->Wait();
}

TEST(JustWorks, RunException) {
  auto tp = yaclib::MakeThreadPool();
  auto result = yaclib::Run(tp,
                            [] {
                              throw std::runtime_error{""};
                            })
                    .Then([] {
                      return 1;
                    })
                    .Get();
  EXPECT_THROW(std::move(result).Ok(), std::runtime_error);
  tp->Stop();
  tp->Wait();
}

TEST(JustWorks, RunError) {
  auto tp = yaclib::MakeThreadPool();
  auto result = yaclib::Run(tp,
                            [] {
                              return yaclib::util::Result<void>{std::error_code{}};
                            })
                    .Then([] {
                      return 1;
                    })
                    .Get();
  EXPECT_THROW(std::move(result).Ok(), yaclib::util::ResultError);
  tp->Stop();
  tp->Wait();
}

TEST(JustWorks, VoidThen) {
  auto tp = yaclib::MakeThreadPool();
  auto f = yaclib::Run(tp, [] {
           }).Then([] {
    return yaclib::util::Result<int>(1);
  });
  EXPECT_EQ(std::move(f).Get().Ok(), 1);
  tp->Stop();
  tp->Wait();
}

TEST(VoidJustWorks, Simple) {
  ValueCheck<void>();
}

TEST(VoidJustWorks, ErrorCode) {
  ErrorsCheck<void>(std::error_code{});
}

TEST(VoidJustWorks, Exception) {
  ErrorsCheck<void>(std::make_exception_ptr(std::runtime_error{""}));
}

TEST(JustWorks, AsyncGetResult) {
  AsyncGetResult(1);
  AsyncGetResult(4);
}

TEST(JustWorks, AsyncRun) {
  auto tp = yaclib::MakeThreadPool(3);
  EXPECT_EQ(tp->Tag(), yaclib::IExecutor::Type::ThreadPool);

  auto good = [&] {
    EXPECT_EQ(yaclib::CurrentThreadPool(), tp);
    return std::string{"Hello!"};
  };

  auto bad = [&] {
    EXPECT_EQ(yaclib::CurrentThreadPool(), tp);
    throw std::logic_error("test");
    return int{};
  };
  auto f1 = yaclib::Run(tp, good);
  auto f2 = yaclib::Run(tp, bad);
  yaclib::Wait(f1, f2);
  EXPECT_TRUE(f1.Ready());
  EXPECT_TRUE(f2.Ready());
  EXPECT_EQ(std::move(f1).Get().Ok(), "Hello!");
  EXPECT_THROW(std::move(f2).Get().Ok(), std::logic_error);

  tp->Stop();
  tp->Wait();
}

TEST(JustWorks, Promise) {
  auto tp = yaclib::MakeThreadPool(2);
  auto [f, p] = yaclib::MakeContract<void>();

  int i = 0;
  auto g = std::move(f).Then(tp, [&] {
    i = 1;
  });
  std::move(p).Set();
  yaclib::Wait(g);
  EXPECT_EQ(i, 1);
  tp->Stop();
  tp->Wait();
}

// Subscribe
TEST(Subscribe, ValueSimple) {
  auto [f, p] = yaclib::MakeContract<int>();

  std::move(p).Set(17);

  bool called = false;

  std::move(f).Subscribe([&called](int result) {
    EXPECT_EQ(result, 17);
    called = true;
  });
  EXPECT_TRUE(called);
}

TEST(Subscribe, ResultSimple) {
  auto [f, p] = yaclib::MakeContract<int>();

  std::move(p).Set(17);

  bool called = false;

  std::move(f).Subscribe([&called](yaclib::util::Result<int> result) {
    EXPECT_EQ(std::move(result).Ok(), 17);
    called = true;
  });

  EXPECT_TRUE(called);
}

TEST(Subscribe, ExceptionSimple) {
  auto [f, p] = yaclib::MakeContract<int>();

  auto result = yaclib::util::Result<int>{};
  result.Set(std::make_exception_ptr(std::runtime_error{""}));

  std::move(p).Set(std::move(result));

  bool called = false;

  std::move(f).Subscribe([&called](yaclib::util::Result<int> result) {
    called = true;
    EXPECT_THROW(std::move(result).Ok(), std::runtime_error);
  });

  EXPECT_TRUE(called);
}

TEST(Subscribe, AsyncSimple) {
  auto tp = yaclib::MakeThreadPool(1);

  auto [f, p] = yaclib::MakeContract<std::string>();

  yaclib_std::atomic<bool> called{false};

  std::move(f).Subscribe([&](yaclib::util::Result<std::string> result) {
    EXPECT_EQ(yaclib::CurrentThreadPool(), tp);
    EXPECT_EQ(std::move(result).Ok(), "Hello!");
    called.store(true, std::memory_order_release);
  });

  EXPECT_FALSE(called.load(std::memory_order_acquire));

  tp->Execute([p = std::move(p)]() mutable {
    std::move(p).Set("Hello!");
  });

  tp->Stop();
  tp->Wait();

  EXPECT_TRUE(called.load(std::memory_order_relaxed));
}

TEST(Subscribe, SubscribeVia) {
  auto tp = yaclib::MakeThreadPool(1);

  auto [f, p] = yaclib::MakeContract<int>();

  std::move(p).Set(17);

  yaclib_std::atomic<bool> called = false;

  auto callback = [&called](yaclib::util::Result<int> result) mutable {
    EXPECT_EQ(std::move(result).Ok(), 17);
    called.store(true);
  };

  // Schedule to thread pool immediately
  std::move(f).Subscribe(tp, callback);

  tp->SoftStop();
  tp->Wait();

  EXPECT_TRUE(called);
}

TEST(Subscribe, SubscribeVia2) {
  auto tp1 = yaclib::MakeThreadPool(1);
  auto tp2 = yaclib::MakeThreadPool(1);

  auto [f, p] = yaclib::MakeContract<int>();

  yaclib_std::atomic<bool> called = false;

  auto callback = [&called](yaclib::util::Result<int> result) mutable {
    EXPECT_EQ(std::move(result).Ok(), 42);
    called.store(true);
  };

  std::move(f).Subscribe(tp2, callback);

  tp1->Execute([p = std::move(p)]() mutable {
    std::move(p).Set(42);
  });

  tp1->SoftStop();
  tp1->Wait();
  tp2->SoftStop();
  tp2->Wait();

  EXPECT_TRUE(called);
}

TEST(Simple, Then) {
  auto [f, p] = yaclib::MakeContract<int>();

  auto g = std::move(f).Then([](int v) {
    return v * 2 + 1;
  });
  std::move(p).Set(3);
  EXPECT_EQ(std::move(g).Get().Ok(), 7);
}

TEST(Simple, ThenThreadPool) {
  auto tp = yaclib::MakeThreadPool(4);

  auto compute = [] {
    return 42;
  };

  auto process = [](int v) -> int {
    return v + 1;
  };

  yaclib::Future<int> f1 = yaclib::Run(tp, compute);

  yaclib::Future<int> f2 = std::move(f1).Then(process);

  EXPECT_EQ(std::move(f2).Get().Ok(), 43);

  tp->Stop();
  tp->Wait();
}

class Unit {};

TEST(Simple, ThenVia) {
  auto tp = yaclib::MakeThreadPool(2);
  auto [f, p] = yaclib::MakeContract<Unit>();

  auto g = std::move(f).Then(tp, [&](Unit) {
    EXPECT_EQ(yaclib::CurrentThreadPool(), tp);
    return 42;
  });

  // Launch
  std::move(p).Set(Unit{});

  EXPECT_EQ(std::move(g).Get().Ok(), 42);

  tp->Stop();
  tp->Wait();
}

TEST(Simple, ThenExecutor) {
  auto tp = yaclib::MakeThreadPool(1);
  {
    std::cerr << " ";
    yaclib::Promise<Unit> p;
    {
      auto f{p.MakeFuture()};
      {
        auto g = std::move(f).Then(tp, [](Unit) {
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
  tp->Stop();
  tp->Wait();
}

TEST(Pipeline, Simple) {
  auto tp = yaclib::MakeThreadPool(1);

  // Pipeline stages:

  auto first = [] {
    return 42;
  };

  auto second = [](int value) {
    return value * 2;
  };

  auto third = [](int value) {
    return value + 1;
  };

  auto f = yaclib::Run(tp, first).Then(second).Then(third);

  EXPECT_EQ(std::move(f).Get().Ok(), 42 * 2 + 1);

  tp->Stop();
  tp->Wait();
}

template <typename T>
class Error : public testing::Test {
 public:
  using Type = T;
};

class ErrorNames {
 public:
  template <typename T>
  static std::string GetName(int i) {
    switch (i) {
      case 0:
        return "std::exception_ptr";
      case 1:
        return "std::error_code";
      default:
        return "unknown";
    }
  }
};

using ErrorTypes = testing::Types<std::exception_ptr, std::error_code>;

TYPED_TEST_SUITE(Error, ErrorTypes, ErrorNames);

TYPED_TEST(Error, Simple1) {
  auto tp = yaclib::MakeThreadPool(4);

  // Pipeline stages:

  auto first = [] {
    return 1;
  };

  auto second = [](int v) {
    std::cout << v << " x 2" << std::endl;
    return v * 2;
  };

  auto third = [](int v) {
    std::cout << v << " + 1" << std::endl;
    return v + 1;
  };

  auto error_handler = [](typename TestFixture::Type) -> int {
    return 42;
  };

  auto last = [&](int v) -> int {
    EXPECT_EQ(yaclib::CurrentThreadPool(), tp);
    return v + 11;
  };

  auto f = yaclib::Run(tp, first).Then(second).Then(third).Then(error_handler).Then(last);

  EXPECT_EQ(std::move(f).Get().Ok(), 14);

  tp->Stop();
  tp->Wait();
}

TYPED_TEST(Error, Simple2) {
  auto tp = yaclib::MakeThreadPool(1);

  // Pipeline stages:

  auto first = []() -> yaclib::util::Result<int> {
    if constexpr (std::is_same_v<typename TestFixture::Type, std::exception_ptr>) {
      throw std::runtime_error("first");
    } else {
      return {std::make_error_code(std::errc::bad_address)};
    }
    return {int{}};
  };

  auto second = [](int v) {
    std::cout << v << " x 2" << std::endl;
    return v * 2;
  };

  auto third = [](int v) {
    std::cout << v << " + 1" << std::endl;
    return v + 1;
  };

  auto error_handler = [](typename TestFixture::Type) -> int {
    return 42;
  };

  auto last = [&](int v) {
    EXPECT_EQ(yaclib::CurrentThreadPool(), tp);
    return v + 11;
  };

  auto pipeline = yaclib::Run(tp, first).Then(second).Then(third).Then(error_handler).Then(last);

  EXPECT_EQ(std::move(pipeline).Get().Value(), 53);
  tp->Stop();
  tp->Wait();
}

class Calculator {
 public:
  Calculator(yaclib::IThreadPoolPtr tp) : _tp(tp) {
  }

  yaclib::Future<int> Increment(int value) {
    return yaclib::Run(_tp, [value] {
      return value + 1;
    });
  }

  yaclib::Future<int> Double(int value) {
    return yaclib::Run(_tp, [value] {
      return value * 2;
    });
  }

 private:
  yaclib::IThreadPoolPtr _tp;
};

TEST(AsyncThen, Simple) {
  auto tp = yaclib::MakeThreadPool(4);

  Calculator calculator(tp);

  auto pipeline = calculator.Increment(1)
                      .Then([&](int value) {
                        return calculator.Double(value);
                      })
                      .Then([&](int value) {
                        return calculator.Increment(value);
                      });
  EXPECT_EQ(std::move(pipeline).Get().Value(), 5);

  tp->SoftStop();
  tp->Wait();
}

TYPED_TEST(Error, AsyncThen) {
  using Type = typename TestFixture::Type;
  auto f = yaclib::MakeFuture(32)
               .ThenInline([](Type) {
                 return yaclib::MakeFuture<int>(42);
               })
               .ThenInline([](yaclib::util::Result<int>) {
                 return 2;
               });
  EXPECT_EQ(std::move(f).Get().Ok(), 2);
  f = yaclib::MakeFuture(32)
          .ThenInline([](int) -> yaclib::util::Result<int> {
            if constexpr (std::is_same_v<Type, std::error_code>) {
              return std::make_error_code(std::errc::bad_address);
            } else {
              throw std::runtime_error{""};
            }
            return {0};
          })
          .ThenInline([](int) {
            return yaclib::MakeFuture<double>(1.0);
          })
          .ThenInline([](yaclib::util::Result<double>) {
            return 2;
          });
  EXPECT_EQ(std::move(f).Get().Ok(), 2);
  f = yaclib::MakeFuture(32)
          .ThenInline([](int) -> yaclib::util::Result<int> {
            if constexpr (std::is_same_v<Type, std::error_code>) {
              return std::make_error_code(std::errc::bad_address);
            } else {
              throw std::runtime_error{""};
            }
            return {0};
          })
          .ThenInline([](Type) {
            return yaclib::MakeFuture<int>(2);
          });
  EXPECT_EQ(std::move(f).Get().Ok(), 2);
}

TEST(Pipeline, Simple2) {
  auto tp1 = yaclib::MakeThreadPool(2);
  auto tp2 = yaclib::MakeThreadPool(3);

  auto [f, p] = yaclib::MakeContract<std::string>();

  auto make_stage = [](int index, yaclib::IThreadPoolPtr current_executor) {
    return [index, current_executor](std::string path) {
      EXPECT_EQ(yaclib::CurrentThreadPool(), current_executor);
      std::cout << "At stage " << index << std::endl;
      return path + "->" + std::to_string(index);
    };
  };

  auto almost_there = std::move(f).Then(tp1, make_stage(1, tp1)).Then(make_stage(2, tp1)).Then(tp2, make_stage(3, tp2));

  std::move(p).Set("start");
  yaclib_std::this_thread::sleep_for(100ms);

  auto finally = std::move(almost_there).Then(make_stage(4, tp2)).Then(tp1, make_stage(5, tp1));

  EXPECT_EQ(std::move(finally).Get().Ok(), "start->1->2->3->4->5");
  tp1->Stop();
  tp1->Wait();
  tp2->Stop();
  tp2->Wait();
}

TEST(Simple, MakePromiseContract) {
  class ManualExecutor : public yaclib::IExecutor {
   private:
    yaclib::util::List<yaclib::ITask> _tasks;

   public:
    [[nodiscard]] Type Tag() const final {
      return yaclib::IExecutor::Type::Custom;
    }

    bool Execute(yaclib::ITask& f) noexcept final {
      f.IncRef();
      _tasks.PushBack(&f);
      return true;
    }

    void Drain() {
      while (auto* task = _tasks.PopBack()) {
        task->Call();
        task->DecRef();
      }
    }
  };

  yaclib::util::NothingCounter<ManualExecutor> e{};
  EXPECT_EQ(e.Tag(), yaclib::IExecutor::Type::Custom);
  auto [f, p] = yaclib::MakeContract<int>();
  auto g = std::move(f).Then(&e, [](int _) {
    return _ + 1;
  });
  EXPECT_FALSE(g.Ready());
  std::move(p).Set(3);
  EXPECT_FALSE(g.Ready());
  e.Drain();
  ASSERT_TRUE(g.Ready());
  EXPECT_EQ(4, std::move(g).Get().Ok());
}

static int default_ctor{0};
static int copy_ctor{0};
static int move_ctor{0};
static int copy_assign{0};
static int move_assign{0};
static int dtor{0};

struct Counter {
  static void Init() {
    default_ctor = 0;
    copy_ctor = 0;
    move_ctor = 0;
    copy_assign = 0;
    move_assign = 0;
    dtor = 0;
  }

  Counter() {
    ++default_ctor;
  }
  Counter(const Counter&) {
    ++copy_ctor;
  }
  Counter(Counter&&) {
    ++move_ctor;
  }
  Counter& operator=(Counter&&) {
    ++move_assign;
    return *this;
  }
  Counter& operator=(const Counter&) {
    ++copy_assign;
    return *this;
  }
  ~Counter() {
    ++dtor;
  }
};

struct Foo : Counter {
  int operator()(int x) & {
    return x + 1;
  }
  int operator()(int x) const& {
    return x + 2;
  }
  int operator()(int x) && {
    return x + 3;
  }
  int operator()(int x) const&& {
    return x + 4;
  }
};

struct AsyncFoo : Counter {
  yaclib::Future<int> operator()(int x) & {
    return yaclib::MakeFuture<int>(x + 1);
  }
  yaclib::Future<int> operator()(int x) const& {
    return yaclib::MakeFuture<int>(x + 2);
  }
  yaclib::Future<int> operator()(int x) && {
    return yaclib::MakeFuture<int>(x + 3);
  }
  yaclib::Future<int> operator()(int x) const&& {
    return yaclib::MakeFuture<int>(x + 4);
  }
};

template <typename Type>
void HelpPerfectForwarding() {
  Type foo;
  const Type cfoo;

  // The continuation will be forward-constructed - copied if given as & and
  // moved if given as && - everywhere construction is required.
  // The continuation will be invoked with the same cvref as it is passed.
  Type::Init();  // Type & lvalue
  EXPECT_EQ(101, yaclib::MakeFuture<int>(100).ThenInline(foo).Get().Ok());
  EXPECT_EQ(default_ctor, 0);
  EXPECT_EQ(copy_ctor, 1);
  EXPECT_EQ(move_ctor, 0);
  EXPECT_EQ(copy_assign, 0);
  EXPECT_EQ(move_assign, 0);
  EXPECT_EQ(dtor, 1);

  Type::Init();  // Type && xvalue
  EXPECT_EQ(303, yaclib::MakeFuture<int>(300).ThenInline(std::move(foo)).Get().Ok());
  EXPECT_EQ(default_ctor, 0);
  EXPECT_EQ(copy_ctor, 0);
  EXPECT_EQ(move_ctor, 1);
  EXPECT_EQ(copy_assign, 0);
  EXPECT_EQ(move_assign, 0);
  EXPECT_EQ(dtor, 1);

  Type::Init();  // const Type & lvalue
  EXPECT_EQ(202, yaclib::MakeFuture<int>(200).ThenInline(cfoo).Get().Ok());
  EXPECT_EQ(default_ctor, 0);
  EXPECT_EQ(copy_ctor, 1);
  EXPECT_EQ(move_ctor, 0);
  EXPECT_EQ(copy_assign, 0);
  EXPECT_EQ(move_assign, 0);
  EXPECT_EQ(dtor, 1);

  Type::Init();  // const Type && lvalue
  EXPECT_EQ(404, yaclib::MakeFuture<int>(400).ThenInline(std::move(cfoo)).Get().Ok());
  EXPECT_EQ(default_ctor, 0);
  EXPECT_EQ(copy_ctor, 1);
  EXPECT_EQ(move_ctor, 0);
  EXPECT_EQ(copy_assign, 0);
  EXPECT_EQ(move_assign, 0);
  EXPECT_EQ(dtor, 1);

  Type::Init();  // Type && prvalue
  EXPECT_EQ(303, yaclib::MakeFuture<int>(300).ThenInline(Type{}).Get().Ok());
  EXPECT_EQ(default_ctor, 1);
  EXPECT_EQ(copy_ctor, 0);
  EXPECT_EQ(move_ctor, 1);
  EXPECT_EQ(copy_assign, 0);
  EXPECT_EQ(move_assign, 0);
  EXPECT_EQ(dtor, 2);
}

TEST(PerfectForwarding, InvokeCallbackReturningValueAsRvalue) {
  HelpPerfectForwarding<Foo>();
}

TEST(PerfectForwarding, InvokeCallbackReturningFutureAsRvalue) {
  HelpPerfectForwarding<AsyncFoo>();
}

TEST(Exception, CallbackReturningValue) {
  auto tp = yaclib::MakeThreadPool(1);

  auto f = yaclib::Run(tp,
                       [] {
                         return 1;
                       })
               .Then([](yaclib::util::Result<int>) {
                 throw std::runtime_error{""};
               })
               .Then([](yaclib::util::Result<void> result) {
                 EXPECT_EQ(result.State(), yaclib::util::ResultState::Exception);
                 return 0;
               });
  EXPECT_EQ(std::move(f).Get().Ok(), 0);
  tp->Stop();
  tp->Wait();
}

TEST(Exception, CallbackReturningFuture) {
  auto tp = yaclib::MakeThreadPool(1);

  auto f = yaclib::Run(tp,
                       [] {
                         return 1;
                       })
               .Then([](int) -> yaclib::Future<void> {
                 throw std::runtime_error{""};
               })
               .Then([](yaclib::util::Result<void> result) {
                 EXPECT_EQ(result.State(), yaclib::util::ResultState::Exception);
                 return 0;
               });
  EXPECT_EQ(std::move(f).Get().Ok(), 0);
  tp->Stop();
  tp->Wait();
}

TEST(Simple, SetAndGetRequiresOnlyMove) {
  struct MoveCtorOnly {
    explicit MoveCtorOnly(int id) : id_(id) {
    }
    MoveCtorOnly(MoveCtorOnly&&) = default;
    MoveCtorOnly& operator=(MoveCtorOnly&&) = default;
    MoveCtorOnly(const MoveCtorOnly&) = delete;
    MoveCtorOnly& operator=(MoveCtorOnly const&) = delete;
    int id_;
  };
  auto f = yaclib::MakeFuture<MoveCtorOnly>(MoveCtorOnly(42));
  EXPECT_TRUE(f.Ready());
  auto v = std::move(f).Get().Ok();
  EXPECT_EQ(v.id_, 42);
}

TEST(Future, special) {
  EXPECT_FALSE(std::is_copy_constructible_v<yaclib::Future<int>>);
  EXPECT_FALSE(std::is_copy_assignable_v<yaclib::Future<int>>);
  EXPECT_TRUE(std::is_move_constructible_v<yaclib::Future<int>>);
  EXPECT_TRUE(std::is_move_assignable_v<yaclib::Future<int>>);
}

TEST(Future, ThenError) {
  bool theFlag = false;
  auto flag = [&] {
    theFlag = true;
  };

  // By reference
  {
    auto f = yaclib::MakeFuture<int>(1)
                 .Then([](auto&&) {
                   throw std::runtime_error{""};
                 })
                 .Then([&](auto&&) {
                   flag();
                 });
    EXPECT_TRUE(std::exchange(theFlag, false));
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }

  {
    auto f = yaclib::MakeFuture<int>(5)
                 .Then([](auto&&) {
                   throw std::runtime_error{""};
                 })
                 .Then([&](auto&&) {
                   flag();
                   return yaclib::MakeFuture<int>(6);
                 });
    EXPECT_TRUE(std::exchange(theFlag, false));
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }

  // By value
  {
    auto f = yaclib::MakeFuture<int>(5)
                 .Then([](auto&&) {
                   throw std::runtime_error{""};
                 })
                 .Then([&](auto) {
                   flag();
                 });
    EXPECT_TRUE(std::exchange(theFlag, false));
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }

  {
    auto f = yaclib::MakeFuture<int>(5)
                 .Then([](auto&&) {
                   throw std::runtime_error{""};
                 })
                 .Then([&](auto) {
                   flag();
                   return yaclib::MakeFuture<int>(5);
                 });
    EXPECT_TRUE(std::exchange(theFlag, false));
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }

  // Mutable lambda
  {
    auto f = yaclib::MakeFuture<int>(5)
                 .Then([](auto&&) {
                   throw std::runtime_error{""};
                 })
                 .Then([&](auto&&) mutable {
                   flag();
                 });
    EXPECT_TRUE(std::exchange(theFlag, false));
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }

  {
    auto f = yaclib::MakeFuture<int>(5)
                 .Then([](auto&&) {
                   throw std::runtime_error{""};
                 })
                 .Then([&](auto&&) mutable {
                   flag();
                   return yaclib::MakeFuture<int>(5);
                 });
    EXPECT_TRUE(std::exchange(theFlag, false));
    EXPECT_NO_THROW(std::move(f).Get().Ok());
  }
}

static std::string DoWorkStatic(yaclib::util::Result<std::string>&& t) {
  return std::move(t).Value() + ";static";
}

static std::string DoWorkStaticValue(std::string&& t) {
  return t + ";value";
}

TEST(Future, ThenFunction) {
  struct Worker {
    static std::string DoWorkStatic(yaclib::util::Result<std::string>&& t) {
      return std::move(t).Value() + ";class-static";
    }
  };

  auto f =
      yaclib::MakeFuture<std::string>("start").Then(DoWorkStatic).Then(Worker::DoWorkStatic).Then(DoWorkStaticValue);

  EXPECT_EQ(std::move(f).Get().Value(), "start;static;class-static;value");
}

TEST(Future, CheckReferenceWrapper) {
  int x = 5;
  auto [f, p] = yaclib::MakeContract<std::reference_wrapper<int>>();
  std::move(p).Set(std::ref(x));
}

TEST(Future, CheckConstGet) {
  auto [f, p] = yaclib::MakeContract<int>();
  auto ptr = std::as_const(f).Get();
  EXPECT_FALSE(f.Ready());
  EXPECT_EQ(ptr, nullptr);
  std::move(p).Set(5);
  ptr = std::as_const(f).Get();
  EXPECT_TRUE(f.Ready());
  EXPECT_NE(ptr, nullptr);
}

TEST(Future, InlineCallback) {
  auto [f, p] = yaclib::MakeContract<void>();
  auto f1 = std::move(f).ThenInline([] {
  });
  std::move(std::move(p)).Set();
  EXPECT_TRUE(f1.Ready());
}

TEST(Future, Stopped) {
  auto [f, p] = yaclib::MakeContract<void>();
  bool ready = false;
  auto f1 = std::move(f).ThenInline([&] {
    ready = true;
  });
  std::move(f1).Stop();
  std::move(p).Set();
  EXPECT_FALSE(ready);
}

TEST(Future, StopInFlight) {
  auto tp = yaclib::MakeThreadPool(1);
  auto f = yaclib::Run(tp, [] {
    yaclib_std::this_thread::sleep_for(10ms);
  });
  yaclib_std::this_thread::sleep_for(10ms);
  std::move(f).Stop();
  tp->Stop();
  tp->Wait();
}

TEST(Future, MakeFuture) {
  {
    auto f = yaclib::MakeFuture();
    bool ready = false;
    std::move(f)
        .ThenInline([&] {
          ready = true;
        })
        .Get()
        .Ok();
    EXPECT_TRUE(ready);
  }
  {
    auto f = yaclib::MakeFuture(1.0F);
    bool ready = false;
    std::move(f)
        .ThenInline([&](float) {
          ready = true;
        })
        .Get()
        .Ok();
    EXPECT_TRUE(ready);
  }
  {
    auto f = yaclib::MakeFuture<double>(1.0F);
    bool ready = false;
    std::move(f)
        .ThenInline([&](double) {
          ready = true;
        })
        .Get()
        .Ok();
    EXPECT_TRUE(ready);
  }
}

TEST(BruhTestCov, BaseCoreCall) {
  // This test needed only for stupid test coverage info
  yaclib::Promise<int> p;
  auto& core = static_cast<yaclib::detail::InlineCore&>(*p.GetCore());
  core.Call();
  core.yaclib::detail::InlineCore::Cancel();
}

}  // namespace
