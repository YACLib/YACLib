#pragma once

#include <yaclib/fault/detail/fiber/queue.hpp>

namespace yaclib::detail::fiber {

class RecursiveMutex {
 public:
  RecursiveMutex() = default;
  ~RecursiveMutex() noexcept = default;

  RecursiveMutex(const RecursiveMutex&) = delete;
  RecursiveMutex& operator=(const RecursiveMutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

  using native_handle_type = void*;

  inline native_handle_type native_handle();

 protected:
  void LockHelper();

  FiberQueue _queue;
  FiberBase::Id _owner_id{0};
  std::uint32_t _occupied_count{0};
};

}  // namespace yaclib::detail::fiber
