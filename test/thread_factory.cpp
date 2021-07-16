#include <container/intrusive_list.hpp>

#include <yaclib/executor/thread_factory.hpp>

#include <atomic>
#include <cstdlib>
#include <thread>

#include <gtest/gtest.h>

namespace {

constexpr bool kSanitizer{false};

// TODO(kononovk): usage better random
static auto init_rand = [] {
  srand(time(0));
  return 0;
}();

using namespace yaclib;

using ThreadsContainter = container::intrusive::List<executor::IThread>;

constexpr auto kDoNothing = [] {
};

template <bool SingleThreaded = true>
void MakeFactoryEmpty(executor::IThreadFactoryPtr factory,
                      size_t acquire_threads, ThreadsContainter& threads) {
  for (size_t i = 0; i != acquire_threads; ++i) {
    auto thread_ptr = factory->Acquire(kDoNothing).release();
    EXPECT_NE(thread_ptr, nullptr);
    if (rand() % 2 == 0) {
      threads.PushBack(thread_ptr);
    } else {
      threads.PushFront(thread_ptr);
    }
  }
  if constexpr (SingleThreaded) {
    EXPECT_EQ(factory->Acquire(kDoNothing), nullptr);
  }
}

void MakeFactoryAvailable(executor::IThreadFactoryPtr factory,
                          size_t release_threads, ThreadsContainter& threads) {
  while (release_threads != 0 && !threads.IsEmpty()) {
    auto release_thread =
        rand() % 2 == 0 ? threads.PopBack() : threads.PopFront();
    release_thread->Join();
    factory->Release(executor::IThreadPtr{release_thread});
    --release_threads;
  }
  EXPECT_EQ(release_threads, 0);
}

template <bool SingleThreaded = true>
void run_tests(size_t iter_count, executor::IThreadFactoryPtr factory,
               size_t max_thread_count) {
  ThreadsContainter threads;
  size_t acquire_counter = max_thread_count;
  size_t release_counter = 0;

  for (size_t i = 0; i < iter_count; ++i) {
    MakeFactoryEmpty<SingleThreaded>(factory, acquire_counter, threads);
    release_counter = 1 + rand() % max_thread_count;
    MakeFactoryAvailable(factory, release_counter, threads);
    acquire_counter = release_counter;
  }
  release_counter = max_thread_count - acquire_counter;
  MakeFactoryAvailable(factory, release_counter, threads);
}

GTEST_TEST(single_threaded, simple) {
  size_t counter{0};
  auto factory = executor::CreateThreadFactory(0, 1);
  auto thread = factory->Acquire([&counter] {
    counter = 1;
  });
  thread->Join();
  EXPECT_EQ(counter, 1);
  EXPECT_EQ(factory->Acquire(kDoNothing), nullptr);
  factory->Release(std::move(thread));
}

GTEST_TEST(single_threaded, complex) {
  static const size_t kMaxThreadCount =
      (kSanitizer ? 1 : 4) * std::thread::hardware_concurrency();
  static const size_t kIterCount = kSanitizer ? 10 : (100 + rand() % 100);
  auto factory = executor::CreateThreadFactory(0, kMaxThreadCount);

  run_tests(kIterCount, factory, kMaxThreadCount);
}

GTEST_TEST(multi_threaded, simple) {
  EXPECT_GE(std::thread::hardware_concurrency(), 2);
  static const size_t kThreadsCount =
      kSanitizer ? 2 : (2 + rand() % (std::thread::hardware_concurrency() - 1));
  static const size_t kMaxThreadCount =
      kThreadsCount * std::thread::hardware_concurrency();
  static const size_t kIterCount = kSanitizer ? 10 : (100 + rand() % 100);

  auto test_threads = executor::CreateThreadFactory(0, kThreadsCount);

  auto factory = executor::CreateThreadFactory(0, kMaxThreadCount);

  ThreadsContainter threads;
  size_t max_threads = 0;
  size_t remaining_threads = kMaxThreadCount;

  for (size_t i = 0; i != kThreadsCount; ++i) {
    max_threads = 1 + rand() % (std::thread::hardware_concurrency() * 2);
    auto todo_threads = kThreadsCount - (i + 1);
    max_threads = std::min(max_threads, remaining_threads - todo_threads);
    ASSERT_TRUE(max_threads != 0);

    auto thread_func = [&factory, max_threads] {
      run_tests<false>(kIterCount, factory, max_threads);
    };
    threads.PushBack(test_threads->Acquire(thread_func).release());

    remaining_threads -= max_threads;
  }

  while (!threads.IsEmpty()) {
    auto release_thread = executor::IThreadPtr{threads.PopFront()};
    release_thread->Join();
    test_threads->Release(std::move(release_thread));
  }
}

}  // namespace
