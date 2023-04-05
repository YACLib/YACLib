#include "yaclib/async/wait.hpp"

#include <util/async_suite.hpp>
#include <util/time.hpp>

#include <yaclib/coro/channel.hpp>
#include <yaclib/coro/on.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>

#include <utility>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

TEST(Channel, Simple) {
  yaclib::FairThreadPool tp{2};
  yaclib::Channel<int> ch;
  auto producer = [&]() -> yaclib::Future<> {
    co_await On(tp);
    for (int i = 0; i != 100; ++i) {
      ch.Push(i);
    }
    co_return{};
  };
  auto consumer = [&]() -> yaclib::Future<> {
    co_await On(tp);
    for (int i = 0; i != 100; ++i) {
      auto r = co_await ch.Pop();
      std::cout << "on iteration " << i << " received value " << r << std::endl;
    }
    co_return{};
  };
  auto c = consumer();
  auto p = producer();

  yaclib::Wait(c, p);

  tp.HardStop();
  tp.Wait();
}

/*
yaclib::Channel<int> produceShit() {
  for (int i = 0; i != 10; ++i) {
    // co_await some_compute;
    co_yield 10;
  }
  co_return {};
}

 static yaclib::Channel<int> ch;

 yaclib::Future<> produceShit(yaclib::Channel<int>& ch){
   for (int i = 0; i != 10; ++i) {
     // co_await some_compute;
     co_await ch.Push(10);
   }
   co_return {};
 }
 produceShit().Detach();
*/

TEST(Channel, SimpleBounded) {
  yaclib::FairThreadPool tp{2};
  yaclib::BoundedChannel<int> ch{25};
  auto producer = [&]() -> yaclib::Future<> {
    co_await On(tp);
    for (int i = 0; i != 100; ++i) {
      co_await ch.Push(i);
    }
    co_return{};
  };
  auto consumer = [&]() -> yaclib::Future<> {
    co_await On(tp);
    for (int i = 0; i != 100; ++i) {
      auto r = co_await ch.Pop();
      std::cout << "on iteration " << i << " received value " << r << std::endl;
    }
    co_return{};
  };
  auto c = consumer();
  auto p = producer();

  yaclib::Wait(c, p);

  tp.HardStop();
  tp.Wait();
}

}  // namespace
}  // namespace test
