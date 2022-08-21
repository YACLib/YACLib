#include <yaclib/exe/job.hpp>
#include <yaclib/util/detail/node.hpp>
#include <yaclib/util/detail/shared_func.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace test {
namespace {

int counter = 0;
void AddOne() {
  ++counter;
}

struct Idle {
  void DoNothing() {
    _mut_method_called = true;
  }

  void DoNothingForceMut() {
    _force_mut_method_called = true;
  }

  void DoNothing() const {
    _const_method_called = true;
  }

  void DoNothingForceConst() const {
    _force_const_method_called = true;
  }

  bool _mut_method_called{false};
  bool _force_mut_method_called{false};
  bool mutable _const_method_called{false};
  bool mutable _force_const_method_called{false};
};

TEST(function_ptr, function_ptr) {
  counter = 0;
  auto task = yaclib::MakeFunc(&AddOne);
  EXPECT_EQ(counter, 0);
  task->Call();
  EXPECT_EQ(counter, 1);
  task->Call();
  EXPECT_EQ(counter, 2);

  task = yaclib::MakeFunc(&AddOne);
  EXPECT_EQ(counter, 2);
  task->Call();
  EXPECT_EQ(counter, 3);
}

TEST(lambda, lvalue) {
  std::size_t value = 0;
  auto lambda = [&] {
    ++value;
  };
  auto task = yaclib::MakeFunc(lambda);
  EXPECT_EQ(value, 0);
  task->Call();
  EXPECT_EQ(value, 1);
  task->Call();
  EXPECT_EQ(value, 2);

  task = yaclib::MakeFunc([&] {
    ++value;
  });
  EXPECT_EQ(value, 2);
  task->Call();
  EXPECT_EQ(value, 3);
}

TEST(lambda, rvalue) {
  std::size_t value = 0;
  auto task = yaclib::MakeFunc([&] {
    ++value;
  });
  EXPECT_EQ(value, 0);
  task->Call();
  EXPECT_EQ(value, 1);
  task->Call();
  EXPECT_EQ(value, 2);
}

TEST(std_function, lvalue) {
  int value = 0;
  const std::function<void()> fun1{[&] {
    ++value;
  }};
  auto task = yaclib::MakeFunc(fun1);
  EXPECT_EQ(value, 0);
  task->Call();
  EXPECT_EQ(value, 1);
  task->Call();
  EXPECT_EQ(value, 2);

  std::function<void()> fun2 = [&] {
    --value;
  };
  task = yaclib::MakeFunc(fun2);
  EXPECT_EQ(value, 2);
  task->Call();
  EXPECT_EQ(value, 1);
}

TEST(std_function, rvalue) {
  int value = 0;
  auto task = yaclib::MakeFunc(std::function<void()>([&] {
    ++value;
  }));
  EXPECT_EQ(value, 0);
  task->Call();
  EXPECT_EQ(value, 1);
  task->Call();
  EXPECT_EQ(value, 2);

  std::function<void()> fun = [&] {
    --value;
  };
  task = yaclib::MakeFunc(std::move(fun));
  EXPECT_EQ(value, 2);
  task->Call();
  EXPECT_EQ(value, 1);
}

TEST(member_function, mut_obj_mut_method) {
  Idle idle;
  auto task = yaclib::MakeFunc([&idle] {
    idle.DoNothingForceMut();
  });
  task->Call();
  EXPECT_EQ(idle._force_mut_method_called, true);

  idle = Idle{};
  task = yaclib::MakeFunc([&idle] {
    idle.DoNothing();
  });
  task->Call();
  EXPECT_EQ(idle._mut_method_called, true);
}

TEST(member_function, mut_obj_const_method) {
  Idle idle;
  auto task = yaclib::MakeFunc([&idle] {
    idle.DoNothingForceConst();
  });
  task->Call();
  EXPECT_EQ(idle._force_const_method_called, true);
}

TEST(member_function, const_obj_const_method) {
  const Idle const_idle;
  auto task = yaclib::MakeFunc([&const_idle] {
    const_idle.DoNothing();
  });
  task->Call();
  EXPECT_EQ(const_idle._const_method_called, true);
}

}  // namespace
}  // namespace test
