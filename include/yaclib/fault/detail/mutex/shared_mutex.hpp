#pragma once

#include <yaclib/fault/detail/inject_fault.hpp>
#include <yaclib/log.hpp>

#include <shared_mutex>

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
};

}  // namespace yaclib::detail
