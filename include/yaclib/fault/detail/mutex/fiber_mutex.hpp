#pragma once

#include <yaclib/fault/detail/fiber/queue.hpp>
#include <yaclib/fault/detail/inject_fault.hpp>

#include <mutex>

namespace yaclib::detail {

class FiberMutex {
 public:
  FiberMutex() = default;
  ~FiberMutex() noexcept = default;

  FiberMutex(const FiberMutex&) = delete;
  FiberMutex& operator=(const FiberMutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

  using native_handle_type = std::mutex::native_handle_type;

  inline native_handle_type native_handle();

  void LockNoInject();
  void UnlockNoInject() noexcept;

 private:
  FiberQueue _queue;
  bool _occupied{false};
};

}  // namespace yaclib::detail
