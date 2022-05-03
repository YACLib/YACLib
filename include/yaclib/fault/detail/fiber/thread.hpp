#pragma once

#include <yaclib/fault/detail/fiber/fiber.hpp>
#include <yaclib/fault/detail/fiber/queue.hpp>
#include <yaclib/fault/detail/fiber/scheduler.hpp>
#include <yaclib/log.hpp>
#include <yaclib/util/detail/shared_func.hpp>

#include <functional>
#include <thread>

namespace yaclib::detail::fiber {

class Thread {
 public:
  Thread(const Thread&) = delete;
  Thread& operator=(const Thread&) = delete;

  using id = FiberBase::Id;
  using native_handle_type = std::thread::native_handle_type;

  template <typename... Args>
  inline explicit Thread(Args&&... args) : _impl(new Fiber<Args...>(std::forward<Args>(args)...)) {
    fault::Scheduler::GetScheduler()->Schedule(_impl);
  }

  Thread() noexcept;
  Thread(Thread&& t) noexcept;
  Thread& operator=(Thread&& t) noexcept;
  ~Thread();

  void swap(Thread& t) noexcept;
  [[nodiscard]] bool joinable() const noexcept;
  void join();
  void detach();

  [[nodiscard]] id get_id() const noexcept;

  native_handle_type native_handle() noexcept;

  static unsigned int hardware_concurrency() noexcept;

  static void SetHardwareConcurrency(unsigned int h_c) noexcept;

 private:
  void AfterJoinOrDetach();

  FiberBase* _impl{nullptr};
  bool _joined_or_detached{false};
};

}  // namespace yaclib::detail::fiber
