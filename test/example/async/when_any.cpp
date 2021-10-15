/**
 * \example when_any.cpp
 * Simple WhenAny examples
 */
#include <yaclib/algo/when_any.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <chrono>
#include <thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

// Any combinator:
// std::vector<Future<T>> -> Future<T>

TEST(Example, WhenAny) {
  auto tp = yaclib::MakeThreadPool(4);

  std::vector<yaclib::Future<int>> futs;

  // Run sync computations in parallel

  for (size_t i = 0; i < 5; ++i) {
    futs.push_back(yaclib::Run(tp, [i]() -> int {
      return i;
    }));
  }

  // Parallel composition
  // Any combinator: std::vector<Future<T>> -> Future<T>
  // Non-blocking!
  yaclib::Future<int> any = yaclib::WhenAny(futs.begin(), futs.size());

  // First value
  std::cout << "Any value: " << std::move(any).Get().Ok() << std::endl;

  tp->Stop();
  tp->Wait();
}
