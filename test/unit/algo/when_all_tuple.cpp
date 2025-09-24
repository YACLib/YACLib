#include <yaclib/async/make.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/async/when_all.hpp>
#include <yaclib/fwd.hpp>

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
  EXPECT_EQ(std::move(f).Get().Error(), yaclib::StopTag{});
}

TEST(WhenAllTuple, FirstFailException) {
  auto f1 = yaclib::MakeFuture<int>(kSetInt);
  auto f2 = yaclib::MakeFuture<std::string>(kSetString);
  auto f3 = yaclib::MakeFuture<bool>(false);
  auto f4 = yaclib::Run([] {
    throw std::runtime_error{""};
  });

  auto f = yaclib::WhenAll(std::move(f1), std::move(f2), std::move(f3), std::move(f4));
  EXPECT_EQ(std::move(f).Get().State(), yaclib::ResultState::Exception);
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
  EXPECT_EQ(std::move(std::get<2>(result)).Error(), yaclib::StopTag{});
  EXPECT_EQ(std::move(std::get<3>(result)).State(), yaclib::ResultState::Exception);
}

}  // namespace
}  // namespace test
