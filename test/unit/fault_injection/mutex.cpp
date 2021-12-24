#include <yaclib/fault/mutex.hpp>
#include <yaclib/fault/shared_mutex.hpp>

#include <gtest/gtest.h>

TEST(mutex, positive_simple) {
  yaclib_std::mutex lock;
  { std::unique_lock kek(lock); }

  { std::unique_lock kek(lock); }
}

TEST(mutex, negative_simple) {
  EXPECT_DEATH(
      {
        yaclib_std::mutex lock;
        lock.unlock();
      },
      "_owner != yaclib::detail::kInvalidThreadId");

  EXPECT_DEATH(
      {
        yaclib_std::mutex lock;
        {
          std::unique_lock kek(lock);
          lock.lock();
        }
      },
      "_owner != yaclib_std::this_thread::get_id()");

  EXPECT_DEATH(
      {
        yaclib_std::mutex lock;
        {
          std::unique_lock kek(lock);
          lock.try_lock();
        }
      },
      "_owner != yaclib_std::this_thread::get_id()");

  EXPECT_DEATH(
      {
        yaclib_std::mutex lock;
        {
          std::unique_lock kek(lock);
          std::thread t([&] {
            lock.unlock();
          });
          t.join();
        }
      },
      "_owner == yaclib_std::this_thread::get_id()");
}

TEST(shared_mutex, positive_simple) {
  yaclib_std::shared_mutex lock;
  { std::unique_lock kek(lock); }

  { std::unique_lock kek(lock); }

  {
    std::shared_lock kek(lock);
    std::thread t([&] {
      std::shared_lock kek2(lock);
    });
    t.join();
  }
}

TEST(shared_mutex, negative_simple) {
  EXPECT_DEATH(
      {
        yaclib_std::shared_mutex lock;
        lock.unlock();
      },
      "_owner != yaclib::detail::kInvalidThreadId");

  EXPECT_DEATH(
      {
        yaclib_std::shared_mutex lock;
        {
          std::unique_lock kek(lock);
          lock.lock();
        }
      },
      "_exclusive_owner != me");

  EXPECT_DEATH(
      {
        yaclib_std::shared_mutex lock;
        {
          std::unique_lock kek(lock);
          lock.try_lock();
        }
      },
      "_exclusive_owner != me");

  EXPECT_DEATH(
      {
        yaclib_std::shared_mutex lock;
        {
          std::unique_lock kek(lock);
          std::thread t([&] {
            lock.unlock();
          });
          t.join();
        }
      },
      "_exclusive_owner == yaclib_std::this_thread::get_id()");

  EXPECT_DEATH(
      {
        yaclib_std::shared_mutex lock;
        lock.unlock_shared();
      },
      "!= _shared_owners.end()");

  EXPECT_DEATH(
      {
        yaclib_std::shared_mutex lock;
        {
          std::unique_lock kek(lock);
          lock.lock_shared();
        }
      },
      "_exclusive_owner != me");

  EXPECT_DEATH(
      {
        yaclib_std::shared_mutex lock;
        {
          std::shared_lock kek(lock);
          lock.try_lock();
        }
      },
      "== _shared_owners.end()");

  EXPECT_DEATH(
      {
        yaclib_std::shared_mutex lock;
        {
          std::shared_lock kek(lock);
          lock.try_lock_shared();
        }
      },
      "== _shared_owners.end()");

  EXPECT_DEATH(
      {
        yaclib_std::shared_mutex lock;
        {
          std::shared_lock kek(lock);
          std::thread t([&] {
            lock.unlock_shared();
          });
          t.join();
        }
      },
      "!= _shared_owners.end()");
}

TEST(recursive_mutex, positive_simple) {
  yaclib_std::recursive_mutex lock;
  { std::unique_lock kek(lock); }

  { std::unique_lock kek(lock); }

  {
    std::unique_lock kek(lock);
    std::unique_lock kek2(lock);
  }
}

TEST(recursive_mutex, negative_simple) {
  EXPECT_DEATH(
      {
        yaclib_std::recursive_mutex lock;
        lock.unlock();
      },
      "_owner != yaclib::detail::kInvalidThreadId");

  EXPECT_DEATH(
      {
        yaclib_std::recursive_mutex lock;
        {
          std::unique_lock kek(lock);
          std::thread t([&] {
            lock.unlock();
          });
          t.join();
        }
      },
      "_owner == yaclib_std::this_thread::get_id()");
}
