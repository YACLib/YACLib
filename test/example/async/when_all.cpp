/**
 * \example when_all.cpp
 * Simple WhenAll examples
 */
#include <yaclib/algo/when_all.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <chrono>
#include <iostream>
#include <type_traits>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

namespace test {
namespace {

using namespace std::chrono_literals;

// All combinator:
// std::vector<Future<T>> -> Future<std::vector<T>>

TEST(Example, WhenAll) {
  auto tp = yaclib::MakeThreadPool(4);

  std::vector<yaclib::FutureOn<int>> futs;

  // Run sync computations in parallel

  futs.reserve(5);
  for (int i = 0; i < 5; ++i) {
    futs.push_back(yaclib::Run(*tp, [i]() -> int {
      return i;
    }));
  }

  // Parallel composition
  // All combinator: std::vector<Future<T>> -> Future<std::vector<T>>
  // Non-blocking!
  yaclib::Future<std::vector<int>> all = WhenAll(futs.begin(), futs.size());

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

}  // namespace
}  // namespace test
