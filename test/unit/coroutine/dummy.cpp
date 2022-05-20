#include <yaclib/async/detail/base_core.hpp>
#include <yaclib/coroutine/detail/promise_type.hpp>
#include <yaclib/util/detail/nope_counter.hpp>
#include <yaclib/util/result.hpp>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(CoroDummy, BaseCoroGetHandle) {
  yaclib::detail::NopeCounter<yaclib::detail::BaseCore> core{yaclib::detail::InlineCore::State::Empty};
  std::ignore = core.GetHandle();
}
}  // namespace
}  // namespace test
