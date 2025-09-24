#include <yaclib/async/join.hpp>
#include <yaclib/async/make.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/fail_policy.hpp>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(Join, AllSuccess) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::MakeFuture(3);

  auto f = yaclib::Join(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Join, IgnoreError) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::Run([] {
    return yaclib::StopTag{};
  });

  auto f = yaclib::Join(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

TEST(Join, ReturnError) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::Run([] {
    return yaclib::StopTag{};
  });

  auto f = yaclib::Join<yaclib::FailPolicy::FirstFail>(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_EQ(std::move(f).Get().Error(), yaclib::StopTag{});
}

TEST(Join, ReturnException) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::Run([] {
    throw std::runtime_error{""};
  });

  auto f = yaclib::Join<yaclib::FailPolicy::FirstFail>(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_EQ(std::move(f).Get().State(), yaclib::ResultState::Exception);
}

TEST(Join, FirstFailNoError) {
  auto f1 = yaclib::MakeFuture(1);
  auto f2 = yaclib::MakeFuture(2);
  auto f3 = yaclib::MakeFuture(3);

  auto f = yaclib::Join<yaclib::FailPolicy::FirstFail>(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_EQ(std::move(f).Get().Value(), yaclib::Unit{});
}

}  // namespace
}  // namespace test
