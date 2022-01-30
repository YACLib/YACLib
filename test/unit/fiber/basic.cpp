#include <yaclib/fiber/coroutine.hpp>
#include <yaclib/util/func.hpp>

#include <thread>

#include <gtest/gtest.h>

TEST(fiber, basic) {
  std::string test;
  auto test_task = yaclib::MakeFunc([&] {
    for (int i = 0; i < 10; i++) {
      int k = i * 8;
      test.append(std::to_string(k));
      yaclib::Coroutine::Yield();
    }
  });
  yaclib::Coroutine coroutine{test_task};
  while (!coroutine.IsCompleted()) {
    test.append("!");
    coroutine.Resume();
  }
  EXPECT_EQ(test, "!0!8!16!24!32!40!48!56!64!72!");
}

TEST(fiber, basic2) {
  std::string test;
  auto test_task1 = yaclib::MakeFunc([&] {
    test.append("1");
    yaclib::Coroutine::Yield();
    test.append("3");
  });

  auto test_task2 = yaclib::MakeFunc([&] {
    test.append("2");
    yaclib::Coroutine::Yield();
    test.append("4");
  });
  yaclib::Coroutine coroutine1{test_task1};
  yaclib::Coroutine coroutine2{test_task2};

  coroutine1.Resume();
  coroutine2.Resume();
  coroutine1.Resume();
  coroutine2.Resume();
  EXPECT_EQ(coroutine1.IsCompleted(), true);
  EXPECT_EQ(coroutine2.IsCompleted(), true);
  EXPECT_EQ(test, "1234");
}

TEST(fiber, basic3) {
  std::string test;
  auto test_task = yaclib::MakeFunc([&test]() {
    test.append("1");
    yaclib::Coroutine::Yield();
    test.append("2");
    yaclib::Coroutine::Yield();
    test.append("3");
    yaclib::Coroutine::Yield();
    test.append("4");
  });

  yaclib::Coroutine coroutine{test_task};

  for (size_t i = 0; i < 4; ++i) {
    std::thread t([&] {
      coroutine.Resume();
    });
    t.join();
  }

  EXPECT_EQ(test, "1234");
  EXPECT_EQ(coroutine.IsCompleted(), true);
}

static int TestFunctionWith2Args(const int* rsi, const int* rdi) {
  auto test_task = yaclib::MakeFunc([&] {
    for (int i = 0; i < 10; i++) {
      yaclib::Coroutine::Yield();
    }
  });
  yaclib::Coroutine coroutine{test_task};

  int iter_count = 0;
  while (!coroutine.IsCompleted()) {
    iter_count++;
    coroutine.Resume();
  }
  EXPECT_EQ(iter_count, 11);
  return *rsi + *rdi;
}

TEST(fiber, basic4) {
  int rsi = 1;
  int rdi = 2;
  int sum = TestFunctionWith2Args(&rsi, &rdi);

  auto test_task = yaclib::MakeFunc([&] {
    for (int i = 0; i < 10; i++) {
      yaclib::Coroutine::Yield();
    }
  });
  yaclib::Coroutine coroutine{test_task};

  int iter_count = 0;
  while (!coroutine.IsCompleted()) {
    iter_count++;
    coroutine.Resume();
  }
  EXPECT_EQ(iter_count, 11);

  EXPECT_EQ(sum, 3);
}
