#include <yaclib/coroutine/detail/promise_type.hpp>

#include <gtest/gtest.h>

TEST(Dummy, DestroyResume) {
  yaclib::detail::Destroy<int, long> d;
  d.await_resume();

  yaclib::detail::Destroy<void, long> d2;
  d.await_resume();
}
