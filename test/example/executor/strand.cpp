/**
 * \example strand.cpp
 * Simple \ref Strand examples
 */

#include <yaclib/executor/strand.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <iostream>

#include <gtest/gtest.h>

using namespace yaclib;

TEST(Example, Strand) {
  std::cout << "Strand" << std::endl;

  auto tp = MakeThreadPool(4);

  auto strand = MakeStrand(tp);

  size_t counter = 0;

  static constexpr size_t kThreads = 5;
  static constexpr size_t kIncrementsPerThread = 12345;

  std::vector<std::thread> threads;

  for (size_t i = 0; i < kThreads; ++i) {
    threads.emplace_back([&]() {
      for (size_t j = 0; j < kIncrementsPerThread; ++j) {
        strand->Execute([&] {
          ++counter;
        });
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  tp->Stop();
  tp->Wait();

  std::cout << "Counter value = " << counter << ", expected " << kThreads * kIncrementsPerThread << std::endl;

  std::cout << std::endl;
}
