#include <yaclib/async/make.hpp>
#include <yaclib/util/result.hpp>

#include <string>
#include <utility>

#include <gtest/gtest.h>

namespace test {
namespace {

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

  Counter() noexcept {
    ++default_ctor;
  }
  Counter(const Counter&) noexcept {
    ++copy_ctor;
  }
  Counter(Counter&&) noexcept {
    ++move_ctor;
  }
  [[maybe_unused]] Counter& operator=(Counter&&) noexcept {
    ++move_assign;
    return *this;
  }
  Counter& operator=(const Counter&) noexcept {
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

static std::string DoWorkStatic(yaclib::Result<std::string>&& t) {
  return std::move(t).Value() + ";static";
}

static std::string DoWorkStaticValue(std::string&& t) {
  return t + ";value";
}

template <bool Inline>
void ThenInlineFunction() {
  struct Worker {
    static std::string DoWorkStatic(yaclib::Result<std::string>&& t) {
      return std::move(t).Value() + ";class-static";
    }
  };

  auto f = yaclib::MakeFuture<std::string>("start")
             .ThenInline(DoWorkStatic)
             .ThenInline(Worker::DoWorkStatic)
             .ThenInline(DoWorkStaticValue);

  EXPECT_EQ(std::move(f).Get().Value(), "start;static;class-static;value");
}
TEST(ThenInline, Function) {
  ThenInlineFunction<true>();
  ThenInlineFunction<false>();
}

}  // namespace
}  // namespace test
