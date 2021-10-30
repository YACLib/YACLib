#include "coroutines/standalone_coroutine_impl.hpp"

#include <thread>

#include <gtest/gtest.h>

template <class T>
class MyTask : public yaclib::util::IFunc {
 public:
  explicit MyTask(T func) : _func(std::move(func)) {
  }

 private:
  void Call() noexcept override {
    _func();
  }

  void IncRef() noexcept override {
  }

  void DecRef() noexcept override {
  }

  T _func;
};

static auto factory = yaclib::MakeStandaloneCoroutineFactory(yaclib::default_allocator_instance);

TEST(coroutine, basic) {
  std::string test;
  auto test_task = MyTask([&] {
    for (int i = 0; i < 10; i++) {
      int k = i * 8;
      test.append(std::to_string(k));
      yaclib::StandaloneCoroutineImpl::Yield();
    }
  });
  auto coroutine = factory->New(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task, false));
  while (!coroutine->IsCompleted()) {
    test.append("!");
    coroutine->Resume();
  }
  EXPECT_EQ(yaclib::default_allocator_instance.GetMinStackSize(), 4096);
  EXPECT_EQ(test, "!0!8!16!24!32!40!48!56!64!72!");
}

TEST(coroutine, basic2) {
  std::string test;
  auto test_task1 = MyTask([&] {
    test.append("1");
    yaclib::StandaloneCoroutineImpl::Yield();
    test.append("3");
  });

  auto test_task2 = MyTask([&] {
    test.append("2");
    yaclib::StandaloneCoroutineImpl::Yield();
    test.append("4");
  });

  auto coroutine1 = factory->New(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task1, false));
  auto coroutine2 = factory->New(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task2, false));

  coroutine1->Resume();
  coroutine2->Resume();
  coroutine1->Resume();
  coroutine2->Resume();
  EXPECT_EQ(coroutine1->IsCompleted(), true);
  EXPECT_EQ(coroutine2->IsCompleted(), true);
  EXPECT_EQ(test, "1234");
}

TEST(coroutine, basic3) {
  std::string test;
  auto test_task = MyTask([&test]() {
    test.append("1");
    yaclib::StandaloneCoroutineImpl::Yield();
    test.append("2");
    yaclib::StandaloneCoroutineImpl::Yield();
    test.append("3");
    yaclib::StandaloneCoroutineImpl::Yield();
    test.append("4");
  });

  auto coroutine = factory->New(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task, false));

  for (size_t i = 0; i < 4; ++i) {
    std::thread t([&]() {
      coroutine->Resume();
    });
    t.join();
  }

  EXPECT_EQ(test, "1234");
  EXPECT_EQ(coroutine->IsCompleted(), true);
}

static int TestFunctionWith2Args(const int* rsi, const int* rdi) {
  auto test_task = MyTask([&] {
    for (int i = 0; i < 10; i++) {
      yaclib::StandaloneCoroutineImpl::Yield();
    }
  });

  auto coroutine = factory->New(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task, false));

  int iter_count = 0;
  while (!coroutine->IsCompleted()) {
    iter_count++;
    coroutine->Resume();
  }
  EXPECT_EQ(iter_count, 11);
  return *rsi + *rdi;
}

TEST(coroutine, basic4) {
  int rsi = 1;
  int rdi = 2;
  int sum = TestFunctionWith2Args(&rsi, &rdi);

  EXPECT_EQ(sum, 3);
}

TEST(coroutine, kek) {
  bool done{false};
  auto test_task = MyTask([&]() {
    while (!done) {
      yaclib::StandaloneCoroutineImpl::Yield();
    }
  });

  auto coroutine = factory->New(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task, false));

  for (size_t i = 0; i != 100'000'000; ++i) {
    coroutine->Resume();
  }
  done = true;
  coroutine->Resume();
  EXPECT_EQ(coroutine->IsCompleted(), true);
}
