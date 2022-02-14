/**
 * \example thread_pool.cpp
 * Simple \ref IThreadPool examples
 */

#include <yaclib/executor/submit.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <iostream>

#include <gtest/gtest.h>

TEST(Example, ThreadPool) {
  std::cout << "ThreadPool" << std::endl;

  auto tp = yaclib::MakeThreadPool(4);

  std::atomic<size_t> counter{0};

  static constexpr std::size_t kIncrements = 100500;

  for (std::size_t i = 0; i < kIncrements; ++i) {
    Submit(tp, [&counter] {
      counter.store(counter.load() + 1);
    });
  }

  tp->Stop();
  tp->Wait();

  std::cout << "Counter value = " << counter << ", expected " << kIncrements << std::endl;

  std::cout << std::endl;
}
