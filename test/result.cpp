#include <yaclib/util/result.hpp>

#include <gtest/gtest.h>

namespace {

using namespace yaclib::util;

struct NotDefaultConstructible {
  NotDefaultConstructible() = delete;
  NotDefaultConstructible(int) {
  }
};

TEST(simple, simple) {
  Result<int> result;
  EXPECT_EQ(result.State(), ResultState::Empty);
  result.Set(5);
  EXPECT_EQ(result.State(), ResultState::Value);
  EXPECT_EQ(std::move(result).Ok(), 5);
}

TEST(simple, not_default_constructible) {
  Result<NotDefaultConstructible> result;
  result.Set(NotDefaultConstructible{5});
}

}  // namespace
