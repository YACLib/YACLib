#pragma once

#include <yaclib/fault/detail/antagonist/inject_fault.hpp>

#include <atomic>
#include <cassert>
#include <mutex>

namespace yaclib::detail {

class Mutex {
 public:
  Mutex() = default;
  ~Mutex() noexcept = default;

  Mutex(const Mutex&) = delete;
  Mutex& operator=(const Mutex&) = delete;

  void lock();
  bool try_lock() noexcept;
  void unlock() noexcept;

  using native_handle_type = std::mutex::native_handle_type;

  inline native_handle_type native_handle();

 private:
  std::mutex _m;
  // TODO(myannyax) yaclib wrapper
  std::atomic<yaclib_std::thread::id> _owner{yaclib::detail::kInvalidThreadId};
};

}  // namespace yaclib::detail
