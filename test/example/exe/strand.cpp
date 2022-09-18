/**
 * \example strand.cpp
 * Simple \ref Strand examples
 */

#include <yaclib/exe/strand.hpp>
#include <yaclib/exe/submit.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/detail/node.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <iostream>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(Example, Strand) {
  std::cout << "Strand" << std::endl;

  yaclib::FairThreadPool tp{4};

  auto strand = MakeStrand(&tp);

  std::size_t counter = 0;

  static constexpr std::size_t kThreads = 5;
  static constexpr std::size_t kIncrementsPerThread = 12345;

  std::vector<yaclib_std::thread> threads;

  for (std::size_t i = 0; i < kThreads; ++i) {
    threads.emplace_back([&]() {
      for (std::size_t j = 0; j < kIncrementsPerThread; ++j) {
        Submit(*strand, [&] {
          ++counter;
        });
      }
    });
  }

  for (auto& t : threads) {
    t.join();
  }

  tp.Stop();
  tp.Wait();

  std::cout << "Counter value = " << counter << ", expected " << kThreads * kIncrementsPerThread << std::endl;

  std::cout << std::endl;
}

}  // namespace
}  // namespace test
