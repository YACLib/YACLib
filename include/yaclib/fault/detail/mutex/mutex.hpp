#pragma once

#include <yaclib/fault/detail/inject_fault.hpp>
#include <yaclib/log_config.hpp>

#include <atomic>
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

  using impl_type = std::mutex;

  impl_type& GetImpl();

  void UpdateOwner(yaclib_std::thread::id);

 private:
  std::mutex _m;
  // TODO(myannyax) yaclib wrapper
  yaclib_std::thread::id _owner = yaclib::detail::kInvalidThreadId;
};

}  // namespace yaclib::detail
