#include <yaclib_std/chrono>
#include <yaclib_std/mutex>
#include <yaclib_std/shared_mutex>
#include <yaclib_std/thread>

#include <gtest/gtest.h>

using namespace std::chrono_literals;

// TODO(myannyax) add better test mutexes not used in code

TEST(FiberSharedMutex, basic) {
  yaclib_std::shared_mutex m;
  std::string res;
  m.lock_shared();
  res += "1";
  yaclib_std::thread t1([&] {
    ASSERT_FALSE(m.try_lock());
    m.lock_shared();
    res += "2";
    m.unlock_shared();
  });

  yaclib_std::thread t2([&] {
    ASSERT_FALSE(m.try_lock());
    ASSERT_TRUE(m.try_lock_shared());
    res += "2";
    m.unlock_shared();
  });

  yaclib_std::thread t3([&] {
    m.lock();
    res += "5";
    m.unlock();
  });
  t1.join();
  t2.join();
  res += "4";
  m.unlock_shared();
  t3.join();
  ASSERT_EQ(res, "12245");
}

TEST(FiberTimedMutex, basic) {
  yaclib_std::timed_mutex m;
  std::string res;
  m.lock();
  yaclib_std::thread t1([&] {
    auto clock = yaclib_std::chrono::steady_clock();
    auto was = clock.now();
    ASSERT_FALSE(m.try_lock_for(10ms));
    ASSERT_GE(clock.now() - was, 10ms);
  });

  t1.join();
  m.unlock();
}

TEST(FiberRecursiveTimedMutex, basic) {
  yaclib_std::recursive_timed_mutex m;
  std::string res;
  m.lock();
  ASSERT_TRUE(m.try_lock());
  yaclib_std::thread t1([&] {
    auto clock = yaclib_std::chrono::steady_clock();
    auto was = clock.now();
    ASSERT_FALSE(m.try_lock_for(10ms));
    auto dur = clock.now() - was;
    ASSERT_GE(dur, 10ms);
    ASSERT_LE(dur, 11ms);
  });

  t1.join();
  m.unlock();
  m.unlock();

  yaclib_std::thread t2([&] {
    ASSERT_TRUE(m.try_lock());
    m.unlock();
  });

  t2.join();
}

TEST(FiberSharedTimedMutex, basic) {
  yaclib_std::shared_timed_mutex m;
  std::string res;
  m.lock();
  yaclib_std::thread t1([&] {
    auto clock = yaclib_std::chrono::steady_clock();
    auto was = clock.now();
    ASSERT_FALSE(m.try_lock_for(10ms));
    auto dur = clock.now() - was;
    ASSERT_GE(dur, 10ms);
    ASSERT_LE(dur, 11ms);
    was = clock.now();
    ASSERT_FALSE(m.try_lock_shared_for(10ms));
    dur = clock.now() - was;
    ASSERT_GE(dur, 10ms);
    ASSERT_LE(dur, 11ms);
  });

  t1.join();
  m.unlock();

  m.lock_shared();

  yaclib_std::thread t2([&] {
    auto clock = yaclib_std::chrono::steady_clock();
    auto was = clock.now();
    ASSERT_FALSE(m.try_lock_for(10ms));
    ASSERT_GE(clock.now() - was, 10ms);
    ASSERT_TRUE(m.try_lock_shared_for(10ms));
  });

  t2.join();
  m.lock_shared();
}
