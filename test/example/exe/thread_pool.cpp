/**
 * \example fair_thread_pool.cpp
 * Simple \ref IThreadPool examples
 */

#include <yaclib/exe/submit.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <cstddef>
#include <iostream>

#include <gtest/gtest.h>

namespace test {
namespace {

TEST(Example, FairThreadPool) {
  std::cout << "FairThreadPool" << std::endl;

  yaclib::FairThreadPool tp{4};

  std::atomic<std::size_t> counter{0};

#if YACLIB_FAULT == 1
  constexpr std::size_t kIncrements = 1000;
#else
  constexpr std::size_t kIncrements = 100500;
#endif

  for (std::size_t i = 0; i < kIncrements; ++i) {
    Submit(tp, [&counter] {
      counter.store(counter.load() + 1);
    });
  }

  tp.Stop();
  tp.Wait();

  std::cout << "Counter value = " << counter << ", expected " << kIncrements << std::endl;

  std::cout << std::endl;
}

}  // namespace
}  // namespace test
