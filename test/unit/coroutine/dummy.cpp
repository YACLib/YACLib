#include <yaclib/coroutine/detail/promise_type.hpp>

#include <gtest/gtest.h>

namespace {
TEST(CoroDummy, DestroyResume) {
  yaclib::detail::Destroy<void, void> sc;
  sc.await_resume();
}

}  // namespace
