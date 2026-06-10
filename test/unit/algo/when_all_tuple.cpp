#include <yaclib/async/make.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/async/when_all.hpp>
#include <yaclib/fwd.hpp>
#include <yaclib/util/result.hpp>

#include <stdexcept>
#include <string_view>

#include <gtest/gtest.h>

namespace test {
namespace {

static constexpr int kSetInt = 3;
static constexpr std::string_view kSetString = "aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa";

TEST(WhenAllTuple, FirstFailAllSuccess) {
  auto f1 = yaclib::MakeFuture<int>(kSetInt);
  auto f2 = yaclib::MakeFuture<std::string>(kSetString);
  auto f3 = yaclib::MakeFuture<bool>(false);
  auto f4 = yaclib::MakeFuture<void>();

  auto f = yaclib::WhenAll(std::move(f1), std::move(f2), std::move(f3), std::move(f4));
  std::tuple<int, std::string, bool, yaclib::Unit> expected{kSetInt, kSetString, false, yaclib::Unit{}};
  EXPECT_EQ(std::move(f).Get().Value(), expected);
}

TEST(WhenAllTuple, FirstFailError) {
  auto f1 = yaclib::MakeFuture<int>(kSetInt);
  auto f2 = yaclib::MakeFuture<std::string>(kSetString);
  auto f3 = yaclib::MakeFuture<bool>(false);
  auto f4 = yaclib::Run([] {
    return yaclib::StopTag{};
  });

  auto f = yaclib::WhenAll(std::move(f1), std::move(f2), std::move(f3), std::move(f4));
  const auto result = std::move(f).Get();
  EXPECT_FALSE(result);
  EXPECT_TRUE(yaclib::IsStop(result.Error()));
}

TEST(WhenAllTuple, FirstFailException) {
  auto f1 = yaclib::MakeFuture<int>(kSetInt);
  auto f2 = yaclib::MakeFuture<std::string>(kSetString);
  auto f3 = yaclib::MakeFuture<bool>(false);
  auto f4 = yaclib::Run([] {
    throw std::runtime_error{""};
  });

  auto f = yaclib::WhenAll(std::move(f1), std::move(f2), std::move(f3), std::move(f4));
  // Exception and Error states are unified now: check error-ness and the concrete exception type
  const auto result = std::move(f).Get();
  EXPECT_FALSE(result);
  EXPECT_FALSE(yaclib::IsStop(result.Error()));
  EXPECT_THROW(std::rethrow_exception(result.Error()), std::runtime_error);
}

TEST(WhenAllTuple, FirstFailTwoErrors) {
  // Both inputs fail: the loser error must be dropped, previously it was accessed as a value
  auto [f1, p1] = yaclib::MakeContract<int>();
  auto [f2, p2] = yaclib::MakeContract<std::string>();
  auto all = yaclib::WhenAll<yaclib::FailPolicy::FirstFail>(std::move(f1), std::move(f2));
  std::move(p1).Set(std::make_exception_ptr(std::runtime_error{"first"}));
  std::move(p2).Set(yaclib::StopTag{});
  auto result = std::move(all).Get();
  EXPECT_FALSE(result);
  EXPECT_THROW(std::rethrow_exception(std::as_const(result).Error()), std::runtime_error);
}

TEST(WhenAllTuple, None) {
  auto f1 = yaclib::MakeFuture<int>(kSetInt);
  auto f2 = yaclib::MakeFuture<std::string>(kSetString);
  auto f3 = yaclib::Run([] {
    return yaclib::StopTag{};
  });
  auto f4 = yaclib::Run([] {
    throw std::runtime_error{""};
  });

  auto f = yaclib::WhenAll<yaclib::FailPolicy::None>(std::move(f1), std::move(f2), std::move(f3), std::move(f4));
  auto result = std::move(f).Get().Value();
  EXPECT_EQ(std::move(std::get<0>(result)).Value(), kSetInt);
  EXPECT_EQ(std::move(std::get<1>(result)).Value(), kSetString);
  const auto& r2 = std::get<2>(result);
  EXPECT_FALSE(r2);
  EXPECT_TRUE(yaclib::IsStop(r2.Error()));
  // Exception and Error states are unified now: check error-ness and the concrete exception type
  const auto& r3 = std::get<3>(result);
  EXPECT_FALSE(r3);
  EXPECT_FALSE(yaclib::IsStop(r3.Error()));
  EXPECT_THROW(std::rethrow_exception(r3.Error()), std::runtime_error);
}

}  // namespace
}  // namespace test
