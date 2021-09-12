/**
 * \example thread_pool.cpp
 * Simple \ref IThreadPool examples
 */

#include <yaclib/executor/thread_pool.hpp>

#include <iostream>

#include <gtest/gtest.h>

TEST(Example, ThreadPool) {
  std::cout << "ThreadPool" << std::endl;

  auto tp = yaclib::executor::MakeThreadPool(4);

  std::atomic<size_t> counter{0};

  static constexpr size_t kIncrements = 100500;

  for (size_t i = 0; i < kIncrements; ++i) {
    tp->Execute([&counter] {
      counter.store(counter.load() + 1);
    });
  }

  tp->Stop();
  tp->Wait();

  std::cout << "Counter value = " << counter << ", expected " << kIncrements << std::endl;

  std::cout << std::endl;
}
