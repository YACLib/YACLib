/**
 * \example when_any.cpp
 * Simple WhenAny examples
 */
#include <yaclib/async/run.hpp>
#include <yaclib/async/when_any.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
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

// Any combinator:
// std::vector<Future<T>> -> Future<T>

TEST(Example, WhenAny) {
  yaclib::FairThreadPool tp{4};

  std::vector<yaclib::FutureOn<int>> futs;

  // Run sync computations in parallel

  for (int i = 0; i < 5; ++i) {
    futs.push_back(yaclib::Run(tp, [i]() -> int {
      return i;
    }));
  }

  // Parallel composition
  // Any combinator: std::vector<Future<T>> -> Future<T>
  // Non-blocking!
  yaclib::Future<int> any = WhenAny(futs.begin(), futs.size());

  // First value
  std::cout << "Any value: " << std::move(any).Get().Ok() << std::endl;

  tp.Stop();
  tp.Wait();
}

}  // namespace
}  // namespace test
