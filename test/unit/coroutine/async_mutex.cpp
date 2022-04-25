#include <util/time.hpp>

#include <yaclib/async/run.hpp>
#include <yaclib/config.hpp>
#include <yaclib/coroutine/async_mutex.hpp>
#include <yaclib/coroutine/await.hpp>
#include <yaclib/coroutine/future_traits.hpp>
#include <yaclib/executor/strand.hpp>
#include <yaclib/executor/submit.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/fault/thread.hpp>

#include <exception>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace {

yaclib::Future<void> coro2(int& sum) {
  sum += 1;
  co_return;
}

TEST(AsyncMutex, JustWorks) {
  using namespace std::chrono_literals;

  yaclib::AsyncMutex m;

  auto tp = yaclib::MakeThreadPool();

  int sum = 0;
  const int N = 10000;
  std::array<yaclib::Future<void>, N> f;
  yaclib_std::atomic<int> done = 0;
  int i = 0;

  auto coro1 = [&]() -> yaclib::Future<void> {
    co_await m.Lock();  // lock
    auto tmp = sum;
    sum = tmp + 1;
    m.SimpleUnlock();  // automatic unlocking
    co_return;
  };

  for (i = 0; i < N; ++i) {
    yaclib::Submit(*tp, [&, i]() {
      f[i] = coro1();
      done.fetch_add(1, std::memory_order_seq_cst);
    });
  }
  std::cout << "After submit in main thread" << std::endl;

  while (!(done.load(std::memory_order_seq_cst) == N)) {
  }

  for (auto&& future : f) {
    std::move(future).Get();
  }

  EXPECT_EQ(N, sum);

  tp->HardStop();
  tp->Wait();
}
}  // namespace
