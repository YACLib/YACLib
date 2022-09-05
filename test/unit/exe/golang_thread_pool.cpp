#include <yaclib/exe/submit.hpp>
#include <yaclib/exe/thread_pool.hpp>

#include <chrono>
#include <iostream>
#include <yaclib_std/thread>
#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

constexpr auto kThreads = 2;
TEST(ThreadPool, JustWorks) {
  auto tp = yaclib::MakeThreadPool(kThreads, nullptr, yaclib::IExecutor::Type::GolangThreadPool);

  bool executed = false;
  yaclib::Submit(*tp, [&] {
    executed = true;
    std::cout << "Hi!" << std::endl;
  });
  yaclib_std::this_thread::sleep_for(0.5s);
  tp->Stop();  // a.k.a Join
  EXPECT_TRUE(executed);
}

TEST(ThreadPool, LocalPushing) {
  using namespace std::chrono_literals;

  auto tp = yaclib::MakeThreadPool(2, nullptr, yaclib::IExecutor::Type::GolangThreadPool);

  yaclib::Submit(*tp, [&] {
    yaclib_std::this_thread::sleep_for(2s);
  });
  yaclib::Submit(*tp, [&] {
    auto current_thread = yaclib_std::this_thread::get_id();
    // Task pushed in current thread
    yaclib::Submit(*tp, [&] {
      auto stealing_thread = yaclib_std::this_thread::get_id();
      EXPECT_EQ(stealing_thread, current_thread);
    });
  });


  yaclib_std::this_thread::sleep_for(0.5s);

  tp->Stop();
}

TEST(ThreadPool, Stealing) {
  auto tp = yaclib::MakeThreadPool(2, nullptr, yaclib::IExecutor::Type::GolangThreadPool);

  yaclib::Submit(*tp, [&] {
    auto current_thread = yaclib_std::this_thread::get_id();
    // Task pushed in current thread
    yaclib::Submit(*tp, [&] {
      auto stealing_thread = yaclib_std::this_thread::get_id();
      EXPECT_NE(stealing_thread, current_thread);
    });
    yaclib_std::this_thread::sleep_for(2s);
  });

  yaclib_std::this_thread::sleep_for(0.5s);
  tp->Stop();
}

}  // namespace
}  // namespace test