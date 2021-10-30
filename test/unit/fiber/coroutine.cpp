#include <yaclib/util/func.hpp>

#include <thread>

#include <gtest/gtest.h>

template <typename Func>
auto MakeMyFunc(Func&& f) {
  return yaclib::util::NothingCounter<yaclib::util::detail::CallImpl<yaclib::util::IFunc, std::decay_t<Func>>>{
      std::forward<Func>(f)};
}

TEST(coroutine, basic) {
  std::string test;
  auto test_task = MakeMyFunc([&] {
    for (int i = 0; i < 10; i++) {
      int k = i * 8;
      test.append(std::to_string(k));
      yaclib::Yield();
    }
  });
  yaclib::Coroutine coroutine{&test_task};
  while (!coroutine.IsCompleted()) {
    test.append("!");
    coroutine.Resume();
  }
  EXPECT_EQ(test, "!0!8!16!24!32!40!48!56!64!72!");
}

TEST(coroutine, basic2) {
  std::string test;
  auto test_task1 = MakeMyFunc([&] {
    test.append("1");
    yaclib::Yield();
    test.append("3");
  });

  auto test_task2 = MakeMyFunc([&] {
    test.append("2");
    yaclib::Yield();
    test.append("4");
  });
  yaclib::Coroutine coroutine1{&test_task1};
  yaclib::Coroutine coroutine2{&test_task2};

  coroutine1.Resume();
  coroutine2.Resume();
  coroutine1.Resume();
  coroutine2.Resume();
  EXPECT_EQ(coroutine1.IsCompleted(), true);
  EXPECT_EQ(coroutine2.IsCompleted(), true);
  EXPECT_EQ(test, "1234");
}

TEST(coroutine, basic3) {
  std::string test;
  auto test_task = MakeMyFunc([&test]() {
    test.append("1");
    yaclib::Yield();
    test.append("2");
    yaclib::Yield();
    test.append("3");
    yaclib::Yield();
    test.append("4");
  });

  yaclib::Coroutine coroutine{&test_task2};

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
  auto test_task = MakeMyFunc([&] {
    for (int i = 0; i < 10; i++) {
      yaclib::Yield();
    }
  });
  yaclib::Coroutine coroutine{&test_task};

  int iter_count = 0;
  while (!coroutine.IsCompleted()) {
    iter_count++;
    coroutine.Resume();
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
