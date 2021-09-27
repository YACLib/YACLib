/**
 * \example when_all.cpp
 * Simple WhenAll examples
 */
#include <yaclib/algo/when_all.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

// All combinator:
// std::vector<Future<T>> -> Future<std::vector<T>>

TEST(Example, WhenAll) {
  auto tp = yaclib::executor::MakeThreadPool(4);

  std::vector<yaclib::async::Future<int>> futs;

  // Run sync computations in parallel

  for (size_t i = 0; i < 5; ++i) {
    futs.push_back(yaclib::async::Run(tp, [i]() -> int {
      return i;
    }));
  }

  // Parallel composition
  // All combinator: std::vector<Future<T>> -> Future<std::vector<T>>
  // Non-blocking!
  yaclib::async::Future<std::vector<int>> all = yaclib::algo::WhenAll(futs.begin(), futs.size());

  // Blocks
  std::vector<int> ints = std::move(all).Get().Ok();

  std::cout << "All ints: ";
  for (auto v : ints) {
    std::cout << v << ", ";
  }
  std::cout << std::endl;
  tp->Stop();
  tp->Wait();
}
