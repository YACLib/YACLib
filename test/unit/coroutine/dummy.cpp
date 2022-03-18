#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/coroutine/detail/suspend_condition.hpp>

#include <gtest/gtest.h>

namespace {
TEST(CoroDummy, SuspendConditionResume) {
  yaclib::detail::SuspendCondition sc(false);
  sc.await_resume();
}
}  // namespace
