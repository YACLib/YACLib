#pragma once

#include <yaclib/fault/detail/antagonist/inject_fault.hpp>
#include <yaclib/fault/log_config.hpp>

#include <shared_mutex>
#include <unordered_set>

namespace yaclib::detail {

class SharedMutex {
 public:
  SharedMutex() = default;
  ~SharedMutex() noexcept = default;

  SharedMutex(const SharedMutex&) = delete;
  SharedMutex& operator=(const SharedMutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

  void lock_shared();
  bool try_lock_shared() noexcept;
  void unlock_shared() noexcept;

  // TODO(myannyax) no handle (my local header has them commented)?

 private:
  std::shared_timed_mutex _m;
  // TODO(myannyax) yaclib wrapper
  std::atomic<yaclib_std::thread::id> _exclusive_owner{yaclib::detail::kInvalidThreadId};
  std::atomic_bool _shared_mode{false};
};

}  // namespace yaclib::detail
