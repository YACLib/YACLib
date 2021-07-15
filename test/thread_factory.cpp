#include <yaclib/executor/thread_factory.hpp>

#include <atomic>
#include <functional>

#include <gtest/gtest.h>

namespace {

using namespace yaclib;
// TODO(kononovk): #2 Add simple single/multi-threaded test

GTEST_TEST(single_threaded, acquire) {
  std::atomic<std::size_t> counter{0};
  auto factory = executor::CreateThreadFactory(0, 1);
  auto thread = factory->Acquire([&counter]() {
    counter.fetch_add(1, std::memory_order_release);
  });
  thread->Join();
  ASSERT_EQ(counter.load(std::memory_order_acquire), 1);
  ASSERT_EQ(factory->Acquire([] {
  }),
            nullptr);
}

GTEST_TEST(single_threaded, release) {
  constexpr std::size_t threads_cnt = 10;
  auto factory = executor::CreateThreadFactory(0, threads_cnt);
  std::atomic<std::size_t> counter{0};
  std::array<executor::IThreadPtr, threads_cnt> thread_ptrs{};

  for (std::size_t i = 0; i < threads_cnt; ++i) {
    auto thread_ptr = factory->Acquire([&] {
      counter++;
    });
    thread_ptrs[i] = std::move(thread_ptr);
  }
  for (std::size_t i = 0; i < threads_cnt; ++i) {
    thread_ptrs[i]->Join();
  }
  ASSERT_EQ(counter.load(), 10);
  ASSERT_TRUE(factory->Acquire([]{}) == nullptr);
  for (std::size_t i = 0; i < threads_cnt; ++i) {
    factory->Release(std::move(thread_ptrs[i]));
    ASSERT_EQ(thread_ptrs[i], nullptr);
  }
  auto thread_ptr = factory->Acquire([]{});
  ASSERT_TRUE(thread_ptr != nullptr);
  thread_ptr->Join();
}

}  // namespace
