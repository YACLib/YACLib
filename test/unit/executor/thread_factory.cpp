#include <util/intrusive_list.hpp>

#include <yaclib/config.hpp>
#include <yaclib/executor/thread_factory.hpp>
#include <yaclib/util/detail/node.hpp>
#include <yaclib/util/func.hpp>
#include <yaclib/util/intrusive_ptr.hpp>

#include <algorithm>
#include <cstdlib>
#include <ctime>
#include <initializer_list>
#include <thread>
#include <utility>

#include <gtest/gtest.h>

namespace {

constexpr bool kSanitizer{YACLIB_SLOWDOWN != 1};

// TODO(kononovk): usage better random
static auto init_rand = [] {
  std::srand(static_cast<unsigned>(std::time(nullptr)));
  return 0;
}();

using namespace yaclib;

using ThreadsContainter = List<IThread>;

const auto kDoNothing = MakeFunc([] {
});

void MakeFactoryEmpty(IThreadFactoryPtr factory, std::size_t acquire_threads, ThreadsContainter& threads) {
  for (std::size_t i = 0; i != acquire_threads; ++i) {
    auto thread_ptr = factory->Acquire(kDoNothing);
    EXPECT_NE(thread_ptr, nullptr);
    if (rand() % 2 == 0) {
      threads.PushBack(thread_ptr);
    } else {
      threads.PushFront(thread_ptr);
    }
  }
}

void MakeFactoryAvailable(IThreadFactoryPtr factory, std::size_t release_threads, ThreadsContainter& threads) {
  while (release_threads != 0 && !threads.IsEmpty()) {
    auto release_thread = rand() % 2 == 0 ? threads.PopBack() : threads.PopFront();
    factory->Release(IThreadPtr{release_thread});
    --release_threads;
  }
  EXPECT_EQ(release_threads, 0);
}

void run_tests(std::size_t iter_count, IThreadFactoryPtr factory, std::size_t max_thread_count) {
  ThreadsContainter threads;
  std::size_t acquire_counter = max_thread_count;
  std::size_t release_counter = 0;

  for (std::size_t i = 0; i < iter_count; ++i) {
    MakeFactoryEmpty(factory, acquire_counter, threads);
    release_counter = 1 + rand() % max_thread_count;
    MakeFactoryAvailable(factory, release_counter, threads);
    acquire_counter = release_counter;
  }
  release_counter = max_thread_count - acquire_counter;
  MakeFactoryAvailable(factory, release_counter, threads);
}

TEST(single_threaded, simple) {
  std::size_t counter{0};
  auto factory = MakeThreadFactory(1);
  auto thread = factory->Acquire(MakeFunc([&counter] {
    counter = 1;
  }));
  factory->Release(std::move(thread));
  EXPECT_EQ(counter, 1);
}

TEST(single_threaded, complex) {
  static const std::size_t kMaxThreadCount = (kSanitizer ? 1 : 4) * std::thread::hardware_concurrency();
  static const std::size_t kIterCount = kSanitizer ? 10 : (100 + rand() % 100);
  for (std::size_t cached_threads : {size_t{0}, kMaxThreadCount / 2, kMaxThreadCount}) {
    auto factory = MakeThreadFactory(MakeThreadFactory(cached_threads), nullptr, nullptr);

    run_tests(kIterCount, factory, kMaxThreadCount);
  }
}

TEST(multi_threaded, simple) {
  EXPECT_GE(std::thread::hardware_concurrency(), 2);
  static const std::size_t kThreadsCount = kSanitizer ? 2 : (2 + rand() % (std::thread::hardware_concurrency() - 1));
  static const std::size_t kMaxThreadCount = kThreadsCount * std::thread::hardware_concurrency();
  static const std::size_t kIterCount = kSanitizer ? 10 : (100 + rand() % 100);

  auto stub = MakeFunc([] {
  });

  for (std::size_t cached_threads : {size_t{0}, kMaxThreadCount / 3, kMaxThreadCount * 2 / 3, kMaxThreadCount}) {
    auto test_threads =
      MakeThreadFactory(MakeThreadFactory(MakeThreadFactory(MakeThreadFactory(kThreadsCount), 1), "stub"),
                        (cached_threads == kMaxThreadCount / 3 ? nullptr : stub),
                        (cached_threads == kMaxThreadCount * 2 / 3 ? nullptr : stub));

    auto factory = MakeThreadFactory(cached_threads);

    ThreadsContainter threads;
    std::size_t max_threads = 0;
    std::size_t remaining_threads = kMaxThreadCount;

    for (std::size_t i = 0; i != kThreadsCount; ++i) {
      max_threads = 1 + rand() % (std::thread::hardware_concurrency() * 2);
      auto todo_threads = kThreadsCount - (i + 1);
      max_threads = std::min(max_threads, remaining_threads - todo_threads);
      ASSERT_TRUE(max_threads != 0);

      const auto thread_func = MakeFunc([&factory, max_threads] {
        run_tests(kIterCount, factory, max_threads);
      });
      threads.PushBack(test_threads->Acquire(thread_func));

      remaining_threads -= max_threads;
    }

    while (!threads.IsEmpty()) {
      auto release_thread = IThreadPtr{threads.PopFront()};
      test_threads->Release(std::move(release_thread));
    }
  }
}

TEST(decorator, priority) {
  // TODO(kononovk): implement test
}
TEST(decorator, name) {
  // TODO(kononovk): implement test
}
TEST(decorator, callback) {
  // TODO(kononovk): implement test
}

}  // namespace
