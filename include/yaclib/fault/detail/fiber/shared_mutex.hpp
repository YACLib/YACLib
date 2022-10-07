#pragma once

#include <yaclib/fault/detail/fiber/queue.hpp>

namespace yaclib::detail::fiber {

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

  bool try_lock_shared();

  void unlock_shared();

  using native_handle_type = void*;

  inline native_handle_type native_handle();

 protected:
  void LockHelper();
  void SharedLockHelper();

  FiberQueue _shared_queue;
  FiberQueue _exclusive_queue;
  std::uint32_t _shared_owners_count{0};
  bool _occupied{false};
  bool _exclusive_mode{false};
};

}  // namespace yaclib::detail::fiber
