#include <util/error_suite.hpp>
#include <util/inline_helper.hpp>

#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/future_splitter.hpp>
#include <yaclib/async/promise.hpp>

#include <exception>
#include <iosfwd>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

namespace test {
namespace {
TEST(SharedPromise, JustWorks) {
  auto [f, p] = yaclib::MakeContract<int>();
  yaclib::FutureSplitter fs{std::move(f)};
  yaclib::Future<int> f1 = fs.GetFuture();
  yaclib::Future<int> f2 = fs.GetFuture();

  EXPECT_FALSE(f1.Ready());
  EXPECT_FALSE(f2.Ready());
  std::move(p).Set(42);
  EXPECT_TRUE(f1.Ready());
  EXPECT_TRUE(f2.Ready());
}
}  // namespace
}  // namespace test
