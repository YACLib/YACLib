#include <yaclib/task.hpp>

#include <functional>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;

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

GTEST_TEST(function_ptr, function_ptr) {
  counter = 0;
  ITaskPtr task = MakeTask(&AddOne);
  EXPECT_EQ(counter, 0);
  task->Call();
  EXPECT_EQ(counter, 1);
  task->Call();
  EXPECT_EQ(counter, 2);

  task = MakeTask(&AddOne);
  EXPECT_EQ(counter, 2);
  task->Call();
  EXPECT_EQ(counter, 3);
}

GTEST_TEST(lambda, lvalue) {
  size_t value = 0;
  auto lambda = [&] {
    ++value;
  };
  ITaskPtr task = MakeTask(lambda);
  EXPECT_EQ(value, 0);
  task->Call();
  EXPECT_EQ(value, 1);
  task->Call();
  EXPECT_EQ(value, 2);

  task = MakeTask([&] {
    ++value;
  });
  EXPECT_EQ(value, 2);
  task->Call();
  EXPECT_EQ(value, 3);
}

GTEST_TEST(lambda, rvalue) {
  size_t value = 0;
  ITaskPtr task = MakeTask([&] {
    ++value;
  });
  EXPECT_EQ(value, 0);
  task->Call();
  EXPECT_EQ(value, 1);
  task->Call();
  EXPECT_EQ(value, 2);
}

GTEST_TEST(std_function, lvalue) {
  int value = 0;
  const std::function<void()> fun1{[&] {
    ++value;
  }};
  auto task = MakeTask(fun1);
  EXPECT_EQ(value, 0);
  task->Call();
  EXPECT_EQ(value, 1);
  task->Call();
  EXPECT_EQ(value, 2);

  std::function<void()> fun2 = [&] {
    --value;
  };
  task = MakeTask(fun2);
  EXPECT_EQ(value, 2);
  task->Call();
  EXPECT_EQ(value, 1);
}

GTEST_TEST(std_function, rvalue) {
  int value = 0;
  auto task = MakeTask(std::function<void()>([&] {
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
  task = MakeTask(std::move(fun));
  EXPECT_EQ(value, 2);
  task->Call();
  EXPECT_EQ(value, 1);
}

GTEST_TEST(member_function, mut_obj_mut_method) {
  Idle idle;
  auto task = MakeTask([&idle] {
    idle.DoNothingForceMut();
  });
  task->Call();
  EXPECT_EQ(idle._force_mut_method_called, true);

  idle = Idle{};
  task = MakeTask([&idle] {
    idle.DoNothing();
  });
  task->Call();
  EXPECT_EQ(idle._mut_method_called, true);
}

GTEST_TEST(member_function, mut_obj_const_method) {
  Idle idle;
  auto task = MakeTask([&idle] {
    idle.DoNothingForceConst();
  });
  task->Call();
  EXPECT_EQ(idle._force_const_method_called, true);
}

GTEST_TEST(member_function, const_obj_const_method) {
  const Idle const_idle;
  auto task = MakeTask([&const_idle] {
    const_idle.DoNothing();
  });
  task->Call();
  EXPECT_EQ(const_idle._const_method_called, true);
}

}  // namespace
