#include <yaclib/algo/wait_for.hpp>
#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/fault/thread.hpp>
#include <yaclib/util/result.hpp>

#include <chrono>
#include <thread>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

TEST(Get, FulFill) {
  {
    auto f = yaclib::MakeFuture(42);
    EXPECT_EQ(42, std::move(f).Get().Ok());
  }
  {
    auto f = yaclib::MakeFuture<int>(43);
    EXPECT_EQ(43, std::move(f).Get().Ok());
  }
}

TEST(Get, PromiseFuture) {
  {
    auto [f, p] = yaclib::MakeContract<int>();
    auto t = yaclib_std::thread([p = std::move(p)]() mutable {
      yaclib_std::this_thread::sleep_for(std::chrono::milliseconds(1));
      std::move(p).Set(43);
    });
    EXPECT_EQ(43, std::move(f).Get().Ok());
    t.join();
  }
  {
    auto [f, p] = yaclib::MakeContract<int>();
    auto t = yaclib_std::thread([p = std::move(p)]() mutable {
      std::move(p).Set(43);
    });
    t.join();
    EXPECT_EQ(43, std::move(f).Get().Ok());
  }
}

TEST(Get, FulFillTimeout) {
  auto f = yaclib::MakeFuture(43);
  EXPECT_TRUE(WaitFor(std::chrono::milliseconds(5), f));
  EXPECT_NE(std::as_const(f).Get(), nullptr);
}

TEST(Get, FulFillTimeout2) {
  auto [f, p] = yaclib::MakeContract<int>();
  EXPECT_FALSE(WaitFor(std::chrono::milliseconds(5), f));
  EXPECT_EQ(std::as_const(f).Get(), nullptr);
  std::move(p).Set(42);
  EXPECT_EQ(42, std::move(f).Get().Ok());
}
