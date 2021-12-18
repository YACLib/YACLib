#pragma once

#include <thread>

namespace yaclib::detail {

class Thread {
 public:
  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

#ifdef YACLIB_FIBER
  // TODO(myannyax)
  using id = size_t;
  using native_handle_type = pthread_t;
#else
  using id = ::std::thread::id;
  using native_handle_type = ::std::thread::native_handle_type;
#endif

  Thread() noexcept;
  explicit Thread(::std::function<void()> routine);
  Thread(Thread&& t) noexcept;

  ~Thread() = default;

  Thread& operator=(Thread&& t) noexcept;

  void swap(Thread& t) noexcept;
  [[nodiscard]] bool joinable() const noexcept;
  void join();
  void detach();

  [[nodiscard]] id get_id() const noexcept;

  native_handle_type native_handle() noexcept;

  static auto hardware_concurrency() noexcept;

 private:
#ifdef YACLIB_FIBER
  // TODO(myannyax) _impl;
#else
  ::std::thread _impl;
#endif
};

extern const Thread::id kInvalidThreadId;

}  // namespace yaclib::detail
