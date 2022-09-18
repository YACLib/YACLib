/**
 * \example simple.cpp
 * Simple Future examples
 */

#include <yaclib/async/contract.hpp>
#include <yaclib/async/future.hpp>
#include <yaclib/async/promise.hpp>
#include <yaclib/async/run.hpp>
#include <yaclib/exe/executor.hpp>
#include <yaclib/exe/strand.hpp>
#include <yaclib/runtime/fair_thread_pool.hpp>
#include <yaclib/util/intrusive_ptr.hpp>
#include <yaclib/util/result.hpp>

#include <chrono>
#include <exception>
#include <iostream>
#include <string>
#include <type_traits>
#include <utility>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(Example, HelloWorld) {
  auto [f, p] = yaclib::MakeContract<int>();

  std::move(p).Set(42);

  EXPECT_TRUE(f.Ready());

  yaclib::Result<int> result = std::move(f).Get();

  EXPECT_EQ(std::move(result).Ok(), 42);
}

TEST(Example, Detach) {
  yaclib::FairThreadPool tp{4};

  auto f = yaclib::Run(tp, [] {
    return 42;
  });

  std::move(f).Detach([](yaclib::Result<int> result) {
    std::cout << "Async result from thread pool: " << std::move(result).Ok() << std::endl;
  });

  EXPECT_TRUE(!f.Valid());

  tp.SoftStop();
  tp.Wait();
}

TEST(Example, Then) {
  yaclib::FairThreadPool tp{4};

  auto compute = [] {
    return 42;
  };

  auto process = [](int r) {
    return r + 1;
  };

  yaclib::FutureOn<int> f1 = yaclib::Run(tp, compute);

  yaclib::FutureOn<int> f2 = std::move(f1).Then(process);

  EXPECT_TRUE(!f1.Valid());

  std::cout << "process(compute()) -> " << std::move(f2).Get().Ok() << std::endl;

  tp.SoftStop();
  tp.Wait();
}

TEST(Example, Pipeline) {
  yaclib::FairThreadPool tp{4};

  // Pipeline stages:

  auto first = []() -> int {
    return 42;
  };

  auto second = [](int r) {
    return r * 2;
  };

  auto third = [](int r) {
    return r + 1;
  };

  auto fourth = [](int r) {
    return std::to_string(r);
  };

  auto last = [](yaclib::Result<std::string> r) {
    std::cout << "Pipeline result: <" << std::move(r).Ok() << ">" << std::endl;
  };

  // Chain pipeline stages and run them in thread pool
  yaclib::Run(tp, first).Then(second).Then(third).Then(fourth).Detach(last);

  tp.SoftStop();
  tp.Wait();
}

class CalculatorService {
 public:
  explicit CalculatorService(yaclib::IExecutorPtr e) : e_(e) {
  }

  yaclib::FutureOn<int> Increment(int value) {
    return yaclib::Run(*e_, [value]() {
      return value + 1;
    });
  }

  yaclib::FutureOn<int> Double(int value) {
    return yaclib::Run(*e_, [value]() {
      return value * 2;
    });
  }

 private:
  yaclib::IExecutorPtr e_;
};

TEST(Example, AsyncPipeline) {
  yaclib::FairThreadPool tp{4};

  CalculatorService calculator(&tp);

  calculator.Increment(1)
    .Then([&](int r) {
      return calculator.Double(r);
    })
    .Then([&](int r) {
      return calculator.Increment(r);
    })
    .Detach([](yaclib::Result<int> r) {
      std::cout << "Result: " << std::move(r).Ok() << std::endl;
    });

  tp.SoftStop();
  tp.Wait();
}

/**
 \todo Add WhenAny example
 */

TEST(Example, Race) {
  yaclib::FairThreadPool tp{1};
  yaclib::FairThreadPool tp2{1};

  auto [f, p] = yaclib::MakeContract<int>();

  yaclib::Run(tp, [p = std::move(p)]() mutable {
    std::move(p).Set(42);
  }).Detach();

  std::move(f).Detach(tp2, [](yaclib::Result<int> /*r*/) {
    std::cout << "Hello from the second thread pool!";
  });

  tp.SoftStop();
  tp.Wait();
  tp2.SoftStop();
  tp2.Wait();
}

TEST(Example, StrandAsync) {
  yaclib::FairThreadPool tp{1};
  auto strand = yaclib::MakeStrand(&tp);

  auto first = []() {
    return 42;
  };

  auto second = [](int r) {
    return r;
  };

  auto third = [](int r) {
    return r + 1;
  };

  yaclib::Run(tp, first)
    .Then(*strand, second)  // Serialized
    .Then(tp, third)
    .Detach([](yaclib::Result<int> r) {
      std::cout << "Final result: " << std::move(r).Value() << std::endl;
    });

  tp.SoftStop();
  tp.Wait();
}
