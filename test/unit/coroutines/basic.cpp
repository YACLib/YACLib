#include <yaclib/coroutines/context/default_allocator.hpp>
#include <yaclib/coroutines/standalone_coroutine.hpp>

#include <gtest/gtest.h>

template <class T>
class MyTask : public yaclib::util::IFunc {
 public:
  explicit MyTask(T func) : _func(std::move(func)) {
  }

  void Call() noexcept override {
    _func();
  }
  void IncRef() noexcept override {
  }
  void DecRef() noexcept override {
  }
 private:
  T _func;
};

TEST(coriutine, basic) {
  std::string test;
  auto test_task = MyTask([&] {
    for (int i = 0; i < 10; i++) {
      test.append(std::to_string(i));
      StandaloneCoroutine::Yield();
    }
  });
  auto allocator = DefaultAllocator();
  allocator.SetMinStackSize(1024);
  auto coroutine = StandaloneCoroutine(allocator, yaclib::util::Ptr<yaclib::util::IFunc>(&test_task, false));
  while(!coroutine.IsCompleted()) {
    coroutine();
  }
  EXPECT_EQ(test, "0123456789");
}
