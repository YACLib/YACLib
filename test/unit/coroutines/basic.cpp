#include <yaclib/coroutines/standalone_coroutine.hpp>

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

TEST(coroutine, basic) {
  std::string test;
  auto test_task = MyTask([&] {
    for (int i = 0; i < 10; i++) {
      int k = i * 8;
      test.append(std::to_string(k));
      yaclib::coroutines::StandaloneCoroutine::Yield();
    }
  });
  auto coroutine = yaclib::coroutines::StandaloneCoroutine(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task, false));
  while (!coroutine.IsCompleted()) {
    test.append("!");
    coroutine();
  }
  EXPECT_EQ(yaclib::coroutines::default_allocator_instance.GetMinStackSize(), 4096);
  EXPECT_EQ(test, "!0!8!16!24!32!40!48!56!64!72!");
}

TEST(coroutine, basic2) {
  std::string test;
  auto test_task1 = MyTask([&] {
    test.append("1");
    yaclib::coroutines::StandaloneCoroutine::Yield();
    test.append("3");
  });

  auto test_task2 = MyTask([&] {
    test.append("2");
    yaclib::coroutines::StandaloneCoroutine::Yield();
    test.append("4");
  });

  auto coroutine1 = yaclib::coroutines::StandaloneCoroutine(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task1, false));
  auto coroutine2 = yaclib::coroutines::StandaloneCoroutine(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task2, false));

  coroutine1();
  coroutine2();
  coroutine1();
  coroutine2();
  EXPECT_EQ(coroutine1.IsCompleted(), true);
  EXPECT_EQ(coroutine2.IsCompleted(), true);
  EXPECT_EQ(test, "1234");
}

TEST(coroutine, basic3) {
  std::string test;
  auto test_task = MyTask([&test]() {
    test.append("1");
    yaclib::coroutines::StandaloneCoroutine::Yield();
    test.append("2");
    yaclib::coroutines::StandaloneCoroutine::Yield();
    test.append("3");
    yaclib::coroutines::StandaloneCoroutine::Yield();
    test.append("4");
  });

  auto coroutine = yaclib::coroutines::StandaloneCoroutine(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task, false));

  for (size_t i = 0; i < 4; ++i) {
    std::thread t([&]() {
      coroutine.Resume();
    });
    t.join();
  }

  EXPECT_EQ(test, "1234");
  EXPECT_EQ(coroutine.IsCompleted(), true);
}

static int TestFunctionWith2Args(const int* rsi, const int* rdi) {
  auto test_task = MyTask([&] {
    for (int i = 0; i < 10; i++) {
      yaclib::coroutines::StandaloneCoroutine::Yield();
    }
  });

  auto coroutine = yaclib::coroutines::StandaloneCoroutine(yaclib::util::Ptr<yaclib::util::IFunc>(&test_task, false));

  int iter_count = 0;
  while (!coroutine.IsCompleted()) {
    iter_count++;
    coroutine();
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
