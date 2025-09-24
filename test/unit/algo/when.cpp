#include <util/when_suite.hpp>
#include <yaclib_std/detail/this_thread.hpp>

#include <yaclib/async/detail/when.hpp>
#include <yaclib/async/make.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/combinator_strategy.hpp>
#include <yaclib/util/result.hpp>

#include <gtest/gtest.h>

namespace test {
namespace {

constexpr int kSetInt = 5;
constexpr std::string_view kSetString = "aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa-aaa";

using namespace std::chrono_literals;

static constexpr auto F = yaclib::FailPolicy::None;
using V = void;
using E = yaclib::StopError;

TYPED_TEST(WhenSuite, UniqueStatic) {
  auto f1 = yaclib::MakeFuture<int>(kSetInt);
  auto f2 = yaclib::MakeFuture<std::string>(kSetString);
  auto f3 = yaclib::MakeFuture<void>();

  auto f = CallWhen<typename TestFixture::Strategy>(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_EQ(std::move(f).Get().Value(), (yaclib::Unit{}));
}

TYPED_TEST(WhenSuite, SharedStatic) {
  auto f1 = yaclib::RunShared([] {
    return kSetInt;
  });
  auto f2 = yaclib::RunShared([] {
    return kSetString;
  });
  auto f3 = yaclib::RunShared([] {
    return;
  });

  auto f = CallWhen<typename TestFixture::Strategy>(std::move(f1), std::move(f2), std::move(f3));
  EXPECT_EQ(std::move(f).Get().Value(), (yaclib::Unit{}));
}

TYPED_TEST(WhenSuite, UniqueDynamic) {
  if constexpr (IsStaticStrategy<typename TestFixture::Strategy>) {
    GTEST_SKIP();
  } else {
    std::vector<yaclib::Future<std::string>> vec;
    for (size_t i = 0; i < 5; ++i) {
      vec.push_back(yaclib::MakeFuture<std::string>(kSetString));
    }

    auto f = CallWhen<typename TestFixture::Strategy>(vec.begin(), vec.size());
    EXPECT_EQ(std::move(f).Get().Value(), (yaclib::Unit{}));
  }
}

TYPED_TEST(WhenSuite, SharedDynamic) {
  if constexpr (IsStaticStrategy<typename TestFixture::Strategy>) {
    GTEST_SKIP();
  } else {
    std::vector<yaclib::SharedFuture<std::string>> vec;
    for (size_t i = 0; i < 5; ++i) {
      vec.push_back(yaclib::RunShared([] {
        return std::string{kSetString};
      }));
    }

    auto f = CallWhen<typename TestFixture::Strategy>(vec.begin(), vec.size());
    EXPECT_EQ(std::move(f).Get().Value(), (yaclib::Unit{}));
  }
}

TYPED_TEST(WhenSuite, AsyncStatic) {
  yaclib::FairThreadPool tp{4};
  auto f1 = yaclib::Run(tp, [] {
    return kSetString;
  });
  auto f2 = yaclib::RunShared(tp, [] {
    return kSetString;
  });
  auto f3 = yaclib::Run(tp, [] {
    yaclib_std::this_thread::sleep_for(100ms);
    return kSetString;
  });
  auto f4 = yaclib::RunShared(tp, [] {
    yaclib_std::this_thread::sleep_for(100ms);
    return kSetString;
  });

  auto f = CallWhen<typename TestFixture::Strategy>(std::move(f1), std::move(f2), std::move(f3), std::move(f4));
  EXPECT_EQ(std::move(f).Get().Value(), (yaclib::Unit{}));

  tp.HardStop();
  tp.Wait();
}

TYPED_TEST(WhenSuite, AsyncUniqueDynamic) {
  if constexpr (IsStaticStrategy<typename TestFixture::Strategy>) {
    GTEST_SKIP();
  } else {
    yaclib::FairThreadPool tp{4};

    std::vector<yaclib::Future<std::string>> vec;
    for (size_t i = 0; i < 5; ++i) {
      vec.push_back(yaclib::Run([] {
        return std::string{kSetString};
      }));
    }
    for (size_t i = 0; i < 5; ++i) {
      vec.push_back(yaclib::Run([] {
        yaclib_std::this_thread::sleep_for(100ms);
        return std::string{kSetString};
      }));
    }

    auto f = CallWhen<typename TestFixture::Strategy>(vec.begin(), vec.size());
    EXPECT_EQ(std::move(f).Get().Value(), (yaclib::Unit{}));

    tp.HardStop();
    tp.Wait();
  }
}

TYPED_TEST(WhenSuite, AsyncSharedDynamic) {
  if constexpr (IsStaticStrategy<typename TestFixture::Strategy>) {
    GTEST_SKIP();
  } else {
    yaclib::FairThreadPool tp{4};

    std::vector<yaclib::SharedFuture<std::string>> vec;
    for (size_t i = 0; i < 5; ++i) {
      vec.push_back(yaclib::RunShared([] {
        return std::string{kSetString};
      }));
    }
    for (size_t i = 0; i < 5; ++i) {
      vec.push_back(yaclib::RunShared([] {
        yaclib_std::this_thread::sleep_for(100ms);
        return std::string{kSetString};
      }));
    }

    auto f = CallWhen<typename TestFixture::Strategy>(vec.begin(), vec.size());
    EXPECT_EQ(std::move(f).Get().Value(), (yaclib::Unit{}));

    tp.HardStop();
    tp.Wait();
  }
}

}  // namespace
}  // namespace test
