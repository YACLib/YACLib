#include <yaclib/coroutines/context/default_allocator.h>
#include <yaclib/coroutines/standalone_coroutine.h>

#include <gtest/gtest.h>

class MyTask : public yaclib::util::IFunc {
  void Call() noexcept override {
    std::cout << "kek";
    StandaloneCoroutine::Yield();
    std::cout << "kek2";
  }
  void IncRef() noexcept override {
  }
  void DecRef() noexcept override {
  }
};

TEST(coriutine, basic) {
  auto kek = MyTask();
  auto& kek2 = dynamic_cast<yaclib::util::IFunc&>(kek);
  auto allocator = DefaultAllocator();
  auto coroutine = StandaloneCoroutine(allocator, yaclib::util::Ptr<yaclib::util::IFunc>(&kek, false));
  coroutine();
  coroutine();
}