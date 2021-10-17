/**
 * \example simple.cpp
 * Simple Future examples
 */

#include <yaclib/async/run.hpp>
#include <yaclib/executor/executor.hpp>
#include <yaclib/executor/serial.hpp>
#include <yaclib/executor/thread_pool.hpp>

#include <atomic>
#include <iostream>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

TEST(Example, HelloWorld) {
  auto [f, p] = yaclib::MakeContract<int>();

  std::move(p).Set(42);

  EXPECT_TRUE(f.Ready());

  yaclib::util::Result<int> result = std::move(f).Get();

  EXPECT_EQ(std::move(result).Ok(), 42);
}

TEST(Example, Subscribe) {
  auto tp = yaclib::MakeThreadPool(4);

  auto f = yaclib::Run(tp, [] {
    return 42;
  });

  std::move(f).Subscribe([](yaclib::util::Result<int> result) {
    std::cout << "Async result from thread pool: " << std::move(result).Ok() << std::endl;
  });

  EXPECT_TRUE(!f.Valid());

  tp->SoftStop();
  tp->Wait();
}

TEST(Example, Then) {
  auto tp = yaclib::MakeThreadPool(4);

  auto compute = [] {
    return 42;
  };

  auto process = [](int r) {
    return r + 1;
  };

  yaclib::Future<int> f1 = yaclib::Run(tp, compute);

  yaclib::Future<int> f2 = std::move(f1).Then(process);

  EXPECT_TRUE(!f1.Valid());

  std::cout << "process(compute()) -> " << std::move(f2).Get().Ok() << std::endl;

  tp->SoftStop();
  tp->Wait();
}

TEST(Example, Pipeline) {
  auto tp = yaclib::MakeThreadPool(4);

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

  auto last = [](yaclib::util::Result<std::string> r) {
    std::cout << "Pipeline result: <" << std::move(r).Ok() << ">" << std::endl;
  };

  // Chain pipeline stages and run them in thread pool
  yaclib::Run(tp, first).Then(second).Then(third).Then(fourth).Subscribe(last);

  tp->SoftStop();
  tp->Wait();
}

class CalculatorService {
 public:
  explicit CalculatorService(yaclib::IExecutorPtr e) : e_(e) {
  }

  yaclib::Future<int> Increment(int value) {
    return yaclib::Run(e_, [value]() {
      return value + 1;
    });
  }

  yaclib::Future<int> Double(int value) {
    return yaclib::Run(e_, [value]() {
      return value * 2;
    });
  }

 private:
  yaclib::IExecutorPtr e_;
};

TEST(Example, AsyncPipeline) {
  auto tp = yaclib::MakeThreadPool(4);

  CalculatorService calculator(tp);

  calculator.Increment(1)
      .Then([&](int r) {
        return calculator.Double(r);
      })
      .Then([&](int r) {
        return calculator.Increment(r);
      })
      .Subscribe([](yaclib::util::Result<int> r) {
        std::cout << "Result: " << std::move(r).Ok() << std::endl;
      });

  tp->SoftStop();
  tp->Wait();
}

/**
 \todo Add WhenAny example
 */

TEST(Example, Race) {
  auto tp = yaclib::MakeThreadPool(1);
  auto tp2 = yaclib::MakeThreadPool(1);

  auto [f, p] = yaclib::MakeContract<int>();

  yaclib::Run(tp, [p = std::move(p)]() mutable {
    std::move(p).Set(42);
  });

  std::move(f).Subscribe(tp2, [](yaclib::util::Result<int> /*r*/) {
    std::cout << "Hello from the second thread pool!";
  });

  tp->SoftStop();
  tp->Wait();
  tp2->SoftStop();
  tp2->Wait();
}

TEST(Example, Serial) {
  auto tp = yaclib::MakeThreadPool(1);
  auto strand = yaclib::MakeSerial(tp);

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
      .Then(strand, second)  // Serialized
      .Then(tp, third)
      .Subscribe([](yaclib::util::Result<int> r) {
        std::cout << "Final result: " << std::move(r).Value() << std::endl;
      });

  tp->SoftStop();
  tp->Wait();
}
